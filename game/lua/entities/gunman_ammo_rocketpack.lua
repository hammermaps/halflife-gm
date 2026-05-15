ENT.Type = "anim"
ENT.Base = "gunman_ammo_base"
ENT.PrintName = "Rocket Pack"
ENT.Category = "Gunman Items"
ENT.Spawnable = true
ENT.AdminOnly = false

ENT.ammoAmount = 6
ENT.ammoType = "gunman_ammo_dmlclip"
ENT.ammoModel = "models/gunman/ammo_rocket_pack.mdl"

AddCSLuaFile()

DEFINE_BASECLASS("gunman_ammo_base")

function ENT:Touch(ent)
	if IsValid(ent) and ent:IsPlayer() then
		if not ent:HasWeapon("gunman_weapon_grenade") then
            self.ammoAmount = self.ammoAmount - 1 
			ent:Give("gunman_weapon_grenade")
		end
	end

	BaseClass.Touch(self, ent)
end
