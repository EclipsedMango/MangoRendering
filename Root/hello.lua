
local SDL_SCANCODE_W = 26
local SDL_SCANCODE_A = 4
local SDL_SCANCODE_S = 22
local SDL_SCANCODE_D = 7

return {
	speed = 5.0,
	zoom_speed = 50.0,
	min_fov = 30.0,
	max_fov = 90.0,
	mouse_capture = true,
	mouse_delta_capture = true,

	_init = function(self)
		print("Movement script initialized for node:", self.node:GetName())
	end,

	_ready = function(self)
		Input.SetMouseCaptureEnabled(true)
	end,

	_process = function(self, delta)
		local pos = self.node:GetPosition()
		local moved = false

		if self.node:GetNodeType() == "CameraNode3d" then
			local current_fov = self.node:Get("fov")
			Input.SetMouseDeltaEnabled(self.mouse_delta_capture)
			local md = Input.GetMouseDelta()
			self.node:AsCamera():Rotate(md.x * 0.08, -md.y * 0.08)

			if Input.IsKeyHeld(Input.Key.Q) then
				current_fov = current_fov - (self.zoom_speed * delta)
			end
			if Input.IsKeyHeld(Input.Key.E) then
				current_fov = current_fov + (self.zoom_speed * delta)
			end

			if current_fov < self.min_fov then current_fov = self.min_fov end
			if current_fov > self.max_fov then current_fov = self.max_fov end

			self.node:Set("fov", current_fov)
		end

		if Input.IsKeyHeld(Input.Key.W) then
			pos.z = pos.z - self.speed * delta
			moved = true
		end
		if Input.IsKeyHeld(Input.Key.S) then
			pos.z = pos.z + self.speed * delta
			moved = true
		end
		if Input.IsKeyHeld(Input.Key.A) then
			pos.x = pos.x - self.speed * delta
			moved = true
		end
		if Input.IsKeyHeld(Input.Key.D) then
			pos.x = pos.x + self.speed * delta
			moved = true
		end
		if Input.IsKeyHeld(Input.Key.SPACE) then
			pos.y = pos.y + self.speed * delta
			moved = true
		end
		if Input.IsKeyHeld(Input.Key.LCTRL) then
			pos.y = pos.y - self.speed * delta
			moved = true
		end

		if Input.IsKeyPressed(Input.Key.ESCAPE) then
			App.Quit()
		end

		if Input.IsKeyPressed(Input.Key.TAB) then
			self.mouse_capture = not self.mouse_capture
			self.mouse_delta_capture = not self.mouse_delta_capture
			Input.SetMouseCaptureEnabled(self.mouse_capture)
		end

		if moved then
			self.node:SetPosition(pos)
		end
	end
}