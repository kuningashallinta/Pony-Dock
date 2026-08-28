local interactions = {}

settings.checkbox("enabled", "Allow interactions", true)
settings.slider("scan", "Seconds between scans", 0.25, 0.05, 2.0)
settings.slider("cooldown", "Shortest cooldown seconds", 3.0, 0.0, 60.0)
settings.slider("patience", "Seconds to wait for the behavior", 2.0, 0.5, 10.0)

local function partner(self, interaction)
	local near = self:nearby(interaction.proximity)

	for i = 1, #near do
		local other = near[i]

		if not other.busy then
			for j = 1, #interaction.targets do
				if interaction.targets[j] == other.pack then
					return other
				end
			end
		end
	end

	return nil
end

local function joint(self, interaction)
	local matches = {}

	for i = 1, #interaction.behaviors do
		local behavior = self.pack.by_id[interaction.behaviors[i]]

		if behavior ~= nil then
			matches[#matches + 1] = behavior
		end
	end

	if #matches == 0 then
		return nil
	end

	return matches[math.random(#matches)]
end

local function commit(self, id)
	self.request = id
	self.interacting = id
	self.joined = false
	self.patience = self:setting("patience")
end

local function settle(self, dt)
	if self.interacting == nil then
		return
	end

	if self.behavior ~= nil and self.behavior.id == self.interacting then
		self.joined = true

		return
	end

	if self.joined then
		self.interacting = nil

		return
	end

	self.patience = self.patience - dt

	if self.patience <= 0 then
		self.interacting = nil
	end
end

local function scan(self)
	local list = self.pack.interactions

	if list == nil then
		return
	end

	for i = 1, #list do
		local interaction = list[i]
		local cooldown = self.cooldowns[interaction.id]

		if interaction.activation == "One" and (cooldown == nil or cooldown <= 0) and math.random() < interaction.chance then
			local other = partner(self, interaction)

			if other ~= nil then
				local behavior = joint(self, interaction)

				if behavior ~= nil and self:invite(other.id, behavior.id) then
					self.cooldowns[interaction.id] = math.max(interaction.delay, self:setting("cooldown"))

					return
				end
			end
		end
	end
end

function interactions.spawn(self)
	self.cooldowns = {}
	self.countdown = 0
end

function interactions.tick(self, dt)
	if self.cooldowns == nil then
		self.cooldowns = {}
	end

	for id, remaining in pairs(self.cooldowns) do
		self.cooldowns[id] = remaining - dt
	end

	settle(self, dt)

	self.busy = self.dragged or self.interacting ~= nil

	if self.busy or self:setting("enabled") == false then
		return
	end

	if self.invited ~= nil then
		local behavior = self.pack.by_id[self.invited.behavior]

		if behavior ~= nil then
			self:invite(self.invited.from, self.invited.behavior)
			commit(self, self.invited.behavior)

			return
		end
	end

	self.countdown = (self.countdown or 0) - dt

	if self.countdown <= 0 then
		self.countdown = self:setting("scan")
		scan(self)
	end
end

return interactions
