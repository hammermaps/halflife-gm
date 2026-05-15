include('shared.lua')

SWEP.setMaterial = {
	[SWEP.FireModes.Pulse]  = Material("gunman/sprites/gausshud1b_pulse"),
	[SWEP.FireModes.Charge] = Material("gunman/sprites/gausshud1b_charge"),
	[SWEP.FireModes.Rapid]  = Material("gunman/sprites/gausshud1b_rapid"),   
	[SWEP.FireModes.Sniper] = Material("gunman/sprites/gausshud1b_sniper"),  
}

SWEP.scaleBeamRing = 0
SWEP.beamStartPos = Vector(0, 0, 0)
SWEP.startPosSniper = Vector(0, 0, 0)
SWEP.pulseBeamEndPos = Vector(0, 0, 0)
SWEP.sniperModeZoom = 0

SWEP.WepSelectIcon = surface.GetTextureID("gunman/vgui/hud_gausspistol")  


function SWEP:DoDrawCrosshair(x, y)
	if self.sniperModeZoom > 0 then return true end

	local textureSize = 96

	local crosshairWidth = 48
	local crosshairHeight = 48

	local screenCenterWidth = x
	local screenCenterHeight = y 

	surface.SetDrawColor(255, 255, 255, 255)
	surface.SetMaterial(Material("gunman/sprites/gausspistol_crosshair"))
	surface.DrawTexturedRect(
		screenCenterWidth - crosshairWidth, screenCenterHeight - crosshairHeight, textureSize, textureSize) 

	return true
end

function SWEP:DrawHUD()
	self.CurrentFireModeHUD = self:GetNW2Int("CurrentFireMode", 1)
	
	self.hudMat = self.setMaterial[self.CurrentFireModeHUD] or self.FireModes[1]

	if self:IsMenuOpen() then
		local baseW, baseH = ScrW(), ScrH() / 4
		local scaleH = baseH / 1440
		local texSizeX, texSizeY = 384, 256

		local x = (baseW - texSizeX) / 2
		local y = (512 * scaleH) + ((baseH - texSizeY) / 2) 

		surface.SetDrawColor(10, 10, 10, 100)
		surface.DrawRect(0, y - 32, ScrW(), texSizeY + 64)

		surface.SetDrawColor(255, 255, 255, 255) 
		surface.SetMaterial(self.hudMat) 
		surface.DrawTexturedRect(x, y, texSizeX, texSizeY)  
	end

	self.sniperModeZoom = self:GetNW2Int("sniperModeZoom", 0) 

	if self.sniperModeZoom > 0 then
		local mats = {
			Material("gunman/sprites/scopezoom"),
			Material("gunman/sprites/scopezoom2"),
			Material("gunman/sprites/scopezoom3"),
		}

		self.sniperModeZoomFrame = self:GetNW2Float("sniperModeZoomFrame", 0)

		local matFrame = (math.floor(self.sniperModeZoomFrame) % 3) + 1
		local mat = mats[matFrame]

		surface.SetMaterial(mat)
		surface.SetDrawColor(100, 255, 100, 255)
	
		local w, h = 128, 128
		local x = (ScrW() / 2) - (w / 2)
		local y = (ScrH() / 2) - (h / 2)
	
		surface.DrawTexturedRect(x, y, w, h)
	end
end

function SWEP:DrawBeamRing(origin, radius, segments, width, ang, col, mat)
    local up = ang:Up()
    local right = ang:Right()

    render.SetMaterial(Material(mat))

    local lastpos

    for i = 0, segments do
        local theta = (i / segments) * math.pi * 2
        local pos = origin + right * math.cos(theta) * radius + up * math.sin(theta) * radius

        if lastpos then
            render.DrawBeam(lastpos, pos, width, 0, 1, col)
        end
        lastpos = pos
    end
end

function SWEP:DrawBeamWithRings(startPos, endPos, alphaDecay)
    local dir = (endPos - startPos):GetNormalized()
    local dist = startPos:Distance(endPos)
    local spacing = 32 

    for i = 0, dist, spacing do
        local pos = startPos + dir * i

        self:DrawBeamRing(
			pos, alphaDecay, 16, 0.5, dir:Angle(),
			Color(255, 255, 255, 500 / alphaDecay), "gunman/sprites/gaussbeam2")
    end

    render.DrawBeam(startPos, endPos, 4, 0, 1, Color(255, 255, 255, 500 / alphaDecay))
end

function SWEP:renderBeam(startPos, endPos, pulseTime)
	if CurTime() < pulseTime then
		render.SetMaterial(Material("gunman/sprites/gaussbeam2"))

		if self.CurrentFireMode == 4 then 
			self.doBeamRings = self:GetNW2Bool("doBeamRings", false)

			if self.doBeamRings then
				self:DrawBeamWithRings(startPos, endPos, self.scaleBeamRing)
			else
				render.DrawBeam(startPos, endPos, 4, 0, 1, Color(255, 255, 255, 400 / self.scaleBeamRing))
			end
		else
			render.DrawBeam(startPos, endPos, 2, 1, 1, Color(255, 255, 255, 255))
		end
	end
end

function SWEP:CustomViewModelDrawn()
	if not IsValid(self.Owner) then return end
	
	local startPosGaussPistol = self:GetViewModelBonePos(45)

	if CurTime() > self.pulseBeamDrawTime then
		self.startPosSniper = self:GetViewModelBonePos(46)
	end

	if self.CurrentFireMode == 4 then
		self.beamStartPos = self.startPosSniper
	else
		self.beamStartPos = startPosGaussPistol
	end

	self.beamEndPos = self:GetNW2Vector("beamEndPos", Vector())
	self.pulseBeamDrawTime = self:GetNW2Float("pulseBeamDrawTime", 0)
	self.scaleBeamRing = self:GetNW2Float("scaleBeamRing", 0)
	self.CurrentFireMode = self:GetNW2Int("CurrentFireMode", 1)
	
	self:renderBeam(self.beamStartPos, self.beamEndPos, self.pulseBeamDrawTime)
end

function SWEP:DrawWorldModel()
	self:DrawModel()

	if not IsValid(self.Owner) then return end

	local weapon = self.Owner:GetActiveWeapon()
	if not IsValid(weapon) then return end

	local muzzle = weapon:LookupAttachment("muzzle")
	local attachment = weapon:GetAttachment(muzzle)
	if not attachment then return end

	local startPosGaussPistol = attachment.Pos 

	local startPos = startPosGaussPistol
	if self.CurrentFireMode == 4 then
		self.startPosSniperWorld = self:GetNW2Vector("startPosSniperWorld", Vector())
		startPos = self.startPosSniperWorld
	end

	self.beamEndPos = self:GetNW2Vector("beamEndPos", Vector())
	self.pulseBeamDrawTime = self:GetNW2Float("pulseBeamDrawTime", 0)
	self.scaleBeamRing = self:GetNW2Float("scaleBeamRing", 0)
	self.CurrentFireMode = self:GetNW2Int("CurrentFireMode", 1)

	self:renderBeam(startPos, self.beamEndPos, self.pulseBeamDrawTime)
end
