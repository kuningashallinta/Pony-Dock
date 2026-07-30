local pd = require("pd")

local core = {}

local MinDuration = 0.25
local MaxLinkDepth = 16
local AscendAngleMin = 30
local AscendAngleMax = 75

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

local function wants(movement)
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
	self.behavior = behavior
	self.link_depth = depth

	local minimum = behavior.duration_min
	local maximum = math.max(behavior.duration_max, minimum)
	self.timer = math.max(MinDuration, minimum + math.random() * (maximum - minimum))

	local horizontal, vertical = wants(behavior.movement)
	local speed = behavior.speed

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
	self.group = 0
	self.vx = 0
	self.vy = 0
	self.x = math.random() * PD.screen.width
	self.y = math.random() * PD.screen.height

	enter(self, pick(self), 0)
end

function core.tick(self, dt)
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
