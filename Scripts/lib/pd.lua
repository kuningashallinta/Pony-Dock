local pd = {}

function pd.sign()
	if math.random() < 0.5 then
		return -1
	end

	return 1
end

function pd.face_velocity(self)
	if self.vx < 0 then
		self:set_facing("left")
	elseif self.vx > 0 then
		self:set_facing("right")
	end
end

function pd.monitor_at(x, y)
	for index = 1, #PD.monitors do
		local area = PD.monitors[index]

		if x >= area.x and x <= area.x + area.width and y >= area.y and y <= area.y + area.height then
			return index
		end
	end

	return nil
end

function pd.area(self)
	local area = PD.monitors[self.monitor]

	if area == nil then
		self.monitor = 1
		area = PD.monitors[1]
	end

	return area
end

function pd.bounce_in_bounds(self)
	local area = pd.area(self)

	if area == nil then
		return
	end

	local left = self.x - self.offset_x
	local top = self.y - self.offset_y

	if left < area.x then
		local crossed = pd.monitor_at(left - 1, self.y)

		if crossed ~= nil then
			self.monitor = crossed
		else
			self.x = area.x + self.offset_x
			self.vx = math.abs(self.vx)
		end
	elseif left + self.width > area.x + area.width then
		local crossed = pd.monitor_at(left + self.width + 1, self.y)

		if crossed ~= nil then
			self.monitor = crossed
		else
			self.x = area.x + area.width - self.width + self.offset_x
			self.vx = -math.abs(self.vx)
		end
	end

	if top < area.y then
		local crossed = pd.monitor_at(self.x, top - 1)

		if crossed ~= nil then
			self.monitor = crossed
		else
			self.y = area.y + self.offset_y
			self.vy = math.abs(self.vy)
		end
	elseif top + self.height > area.y + area.height then
		local crossed = pd.monitor_at(self.x, top + self.height + 1)

		if crossed ~= nil then
			self.monitor = crossed
		else
			self.y = area.y + area.height - self.height + self.offset_y
			self.vy = -math.abs(self.vy)
		end
	end
end

return pd
