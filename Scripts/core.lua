local pd = require("pd")

local core = {}

local MinDuration = 0.25
local MaxLinkDepth = 16
local AscendAngleMin = 30
local AscendAngleMax = 75

core.generation = 0

settings.slider("speed", "Speed multiplier", 1.0, 0.1, 4.0)
settings.checkbox("flying", "Allow flying", true)
settings.dropdown("start", "Spawn position", {"Anywhere", "Ground", "Center"}, "Anywhere")

settings.button("shuffle", "Reshuffle behaviors", function()
	core.generation = core.generation + 1
	log("reshuffling every entity")
end)

local function start_group(self)
	local groups = self.pack.groups

	if groups == nil or #groups == 0 then
		return 0
	end

	return groups[1]
end

local function eligible(self, behavior)
	if behavior.chance <= 0 or behavior.skip then
		return false
	end

	if behavior.group ~= 0 and behavior.group ~= self.group then
		return false
	end

	return true
end

local function pick(self)
	local pool = self.pack.behaviors
	local total = 0

	for index = 1, #pool do
		if eligible(self, pool[index]) then
			total = total + pool[index].chance
		end
	end

	if total <= 0 then
		return pool[1]
	end

	local roll = math.random() * total

	for index = 1, #pool do
		local behavior = pool[index]

		if eligible(self, behavior) then
			roll = roll - behavior.chance

			if roll <= 0 then
				return behavior
			end
		end
	end

	return pool[1]
end

local function wants(self, movement)
	if self:setting("flying") == false then
		return true, false
	end

	if movement == "Horizontal_Only" then
		return true, false
	elseif movement == "Vertical_Only" then
		return false, true
	elseif movement == "Diagonal_Only" then
		return true, true
	elseif movement == "Horizontal_Vertical" then
		if math.random() < 0.5 then
			return true, false
		end

		return false, true
	elseif movement == "Diagonal_Horizontal" then
		return true, math.random() < 0.5
	elseif movement == "Diagonal_Vertical" then
		return math.random() < 0.5, true
	elseif movement == "All" then
		local horizontal = math.random() < 0.5
		local vertical = math.random() < 0.5

		if not horizontal and not vertical then
			horizontal = true
		end

		return horizontal, vertical
	end

	return false, false
end

local function enter(self, behavior, depth)
	if behavior.group ~= 0 then
		self.group = behavior.group
	end

	self.behavior = behavior
	self.link_depth = depth

	local minimum = behavior.duration_min
	local maximum = math.max(behavior.duration_max, minimum)
	self.timer = math.max(MinDuration, minimum + math.random() * (maximum - minimum))

	local horizontal, vertical = wants(self, behavior.movement)
	local speed = behavior.speed * (self:setting("speed") or 1.0)

	self.vx = 0
	self.vy = 0

	if speed > 0 then
		local horizontalSign = pd.sign()
		local verticalSign = pd.sign()

		if vertical then
			local angle = math.rad(AscendAngleMin + math.random() * (AscendAngleMax - AscendAngleMin))
			self.vx = math.cos(angle) * horizontalSign * speed
			self.vy = math.sin(angle) * verticalSign * speed
		elseif horizontal then
			self.vx = horizontalSign * speed
		end
	end

	pd.face_velocity(self)
	self:play(behavior.animation, not behavior.prevent_animation_loop)
end

local function advance(self)
	local linked = self.behavior.linked

	if linked ~= nil and self.link_depth < MaxLinkDepth then
		local next_behavior = self.pack.by_id[linked]

		if next_behavior ~= nil then
			enter(self, next_behavior, self.link_depth + 1)

			return
		end
	end

	enter(self, pick(self), 0)
end

function core.spawn(self)
	self.group = start_group(self)
	self.generation = core.generation
	self.vx = 0
	self.vy = 0

	if self.placed then
		enter(self, pick(self), 0)

		return
	end

	self.placed = true
	self.monitor = 1

	if #PD.monitors > 1 then
		self.monitor = math.random(#PD.monitors)
	end

	local area = PD.monitors[self.monitor]
	local start = self:setting("start")

	if area == nil then
		self.x = math.random() * PD.screen.width
		self.y = math.random() * PD.screen.height
	elseif start == "Ground" then
		self.x = area.x + math.random() * area.width
		self.y = area.y + area.height - self.height + self.offset_y
	elseif start == "Center" then
		self.x = area.x + area.width * 0.5
		self.y = area.y + area.height * 0.5
	else
		self.x = area.x + math.random() * area.width
		self.y = area.y + math.random() * area.height
	end

	enter(self, pick(self), 0)
end

function core.tick(self, dt)
	if self.generation ~= core.generation then
		self.generation = core.generation
		advance(self)

		return
	end

	self.timer = self.timer - dt

	local done = self.timer <= 0

	if self.behavior.prevent_animation_loop and self.animation_finished then
		done = true
	end

	if done then
		advance(self)

		return
	end

	self.x = self.x + self.vx * dt
	self.y = self.y + self.vy * dt

	pd.bounce_in_bounds(self)
	pd.face_velocity(self)
end

return core
