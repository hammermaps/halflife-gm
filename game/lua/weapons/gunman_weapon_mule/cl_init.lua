AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")
include('shared.lua')

SWEP.MenuLabels = {
	Launch = "LAUNCH",
	Flightpath = "FLIGHTPATH",
	Detonation = "DETONATION",
	Payload = "PAYLOAD",
}

SWEP.Launch = {
	"WHEN FIRED",
	"WHEN TARGETED",
}

SWEP.Flightpath = {
	"GUIDED",
	"HOMING",
	"SPIRAL",
}

SWEP.Detonation = {
	"ON IMPACT",
	"TIMED",
	"WHEN TRIPPED",
}

SWEP.Payload = {
	"EXPLOSIVE",
	"CLUSTER",
}

SWEP.WepSelectIcon = surface.GetTextureID("gunman/vgui/hud_mule")  

function SWEP:DoDrawCrosshair(x, y)
	local trace = util.TraceLine({
		start = self.Owner:GetShootPos(), 
		endpos = self.Owner:GetShootPos() + self.Owner:GetForward() * 2048, 
		filter = { self.Owner }
	})

	if self.CurrentFlightPathType == 1 then 
		local dotScale = math.Clamp(8 / trace.Fraction, 6, 36)
		local centerX = ScrW() / 2
		local centerY = ScrH() / 2

		local dotX = centerX - (dotScale / 2) 
		local dotY = centerY - (dotScale / 2)  
		
		surface.SetDrawColor(255, 0, 0, 255)
		surface.SetMaterial(Material("gunman/sprites/laserdot"))
		surface.DrawTexturedRect(dotX, dotY, dotScale, dotScale)
	end
	
	surface.SetDrawColor(255, 255, 255, 255) 

	if self.CurrentFlightPathType ~= 2 then 
		local textureSize = 96

		local crosshairWidth = 48
		local crosshairHeight = 48
	
		local screenCenterWidth = x
		local screenCenterHeight = y 
		
		surface.SetDrawColor(255, 255, 255, 255)
		surface.SetMaterial(Material("gunman/sprites/dml_crosshair"))
		surface.DrawTexturedRect(
			screenCenterWidth - crosshairWidth, screenCenterHeight - crosshairHeight, textureSize, textureSize) 
	end

	if self.CurrentFlightPathType ~= 2 then return end

	if self.homingTarget and self.homingTarget:IsValid() then
		surface.SetMaterial(Material("gunman/sprites/dml_crosshair_locked")) 
	else
		self.resetSound = false
		surface.SetMaterial(Material("gunman/sprites/dml_crosshair_homing")) 
	end

	surface.DrawTexturedRect(ScrW() / 2.05, ScrH() / 2.09, 48, 48)  

	if self.homingRange and self.homingRange:IsValid()  then
		screenPosOuter = self.homingRange:GetPos():ToScreen()
		local moveX = screenPosOuter.x - 64
		local moveY = screenPosOuter.y - 64

		surface.SetMaterial(Material("gunman/sprites/dml_crosshair_locked2"))
		surface.DrawTexturedRect(moveX, moveY, 128, 128)
	end

	if self.homingTarget and self.homingTarget:IsValid() and not self.resetSound then
		self.resetSound = true
		self.Owner:EmitSound("gunman_dml_lock", 100, 50)
	end

	return true
end

function SWEP:CustomDrawHUD()	
	local menuLabels = {
		{ label = self.MenuLabels.Launch, value = self.Launch[self.CurrentLaunchType], offset = 0 },
		{ label = self.MenuLabels.Flightpath, value = self.Flightpath[self.CurrentFlightPathType], offset = 60 },
		{ label = self.MenuLabels.Detonation, value = self.Detonation[self.CurrentDetonationType], offset = 120 },
		{ label = self.MenuLabels.Payload, value = self.Payload[self.CurrentPayloadType], offset = 180 },		
	}

	self:DrawMenuHUD(menuLabels)

	if name == "CHudAmmo" then
        return false  -- Don't draw ammo HUD element
    end
end

function SWEP:DrawWorldModel()
	self.Weapon:DrawModel()
	
	if self.Owner:IsValid() == false then return end

	if self.CurrentFlightPathType ~= 1 then return end
	local Muzzle = self.Owner:GetActiveWeapon():GetAttachment(
		self.Owner:GetActiveWeapon():LookupAttachment("muzzle"))

	local startPos = Muzzle.Pos 
	local endPos = Muzzle.Ang:Right() * 256

	local trace = util.TraceLine({
		start = startPos, 
		endpos = startPos + endPos, 
		filter = { self.Owner }
	})

	local actualPos = trace.HitPos + trace.HitNormal * 4

	render.SetMaterial(Material("gunman/sprites/xbeam1"))
	render.DrawBeam(startPos, actualPos, 1, 8, 8, Color(255, 0, 0))

	if trace.Hit then
		render.SetMaterial(Material("sprites/glow04_noz"))
    	render.DrawSprite(actualPos, 16, 16, Color(255, 0, 0))
	end
end
	
function SWEP:CustomViewModelDrawn()
	if self.homingTarget and self.homingTarget:IsValid() and self.CurrentFlightPathType == 2 then
		self.Owner:GetViewModel():SetSubMaterial(3, "models/gunman/weapons/v_dml/screen3.vmt")
	else
		self.Owner:GetViewModel():SetSubMaterial(3, "models/gunman/weapons/v_dml/screen1.vmt")
	end
end
