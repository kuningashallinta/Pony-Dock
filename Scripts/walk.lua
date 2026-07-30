function tick(position, velocity, dt, bounds_width, bounds_height, sprite_width, sprite_height)
	position.x = position.x + velocity.x * dt

	if position.x < 0 then
		position.x = 0
		velocity.x = -velocity.x
	elseif position.x + sprite_width > bounds_width then
		position.x = bounds_width - sprite_width
		velocity.x = -velocity.x
	end

	position.y = bounds_height - sprite_height
end
