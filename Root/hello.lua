return {
	speed = 90.0,
	rot = 1.0,

	_init = function(self)
		print("Rotate script initialized for node:", self.node:GetName())
	end,

	_physics_process = function(self, delta)
		self.rot = self.rot + 10.0 * self.speed * delta
		self.node:SetRotationEuler(vec3.new(0.0, self.rot, 0.0))
	end
}