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

function pd.bounce_in_bounds(self)
	local left = self.x - self.offset_x
	local top = self.y - self.offset_y
	local width = PD.screen.width
	local height = PD.screen.height

	if left < 0 then
		self.x = self.offset_x
		self.vx = math.abs(self.vx)
	elseif left + self.width > width then
		self.x = width - self.width + self.offset_x
		self.vx = -math.abs(self.vx)
	end

	if top < 0 then
		self.y = self.offset_y
		self.vy = math.abs(self.vy)
	elseif top + self.height > height then
		self.y = height - self.height + self.offset_y
		self.vy = -math.abs(self.vy)
	end
end

return pd
