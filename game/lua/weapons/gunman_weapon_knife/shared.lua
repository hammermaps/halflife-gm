AddCSLuaFile("gunman_shared.lua")
local GunmanShared = include("gunman_shared.lua")

SWEP.Base = "gunman_base"
SWEP.Category = "Gunman Weaponry"
SWEP.Author = "Azzy"
SWEP.PrintName	= "Knife"	
SWEP.Purpose = "That's not a knife, this a knife! ... Including your barehands."	
SWEP.Spawnable = true
SWEP.AdminOnly = false

SWEP.ViewModelFOV = 100
SWEP.ViewModel	= "models/gunman/v_hands.mdl"
SWEP.WorldModel = "models/gunman/w_knife.mdl"

SWEP.BobScale = 1
SWEP.SwayScale = 1

SWEP.Slot = 0
SWEP.SlotPos = 0

SWEP.DrawCrosshair = true
SWEP.DrawAmmo = true

SWEP.HoldType = "melee"

SWEP.Primary.ClipSize = -1
SWEP.Primary.DefaultClip = -1
SWEP.Primary.MaxAmmo = -1
SWEP.Primary.Automatic = true
SWEP.Primary.Ammo = "none"
SWEP.Primary.Damage = GunmanShared.WeaponDamage.Knife.Hands
SWEP.Primary.Spread = 0
SWEP.Primary.TakeAmmo = 0
SWEP.Primary.NumberofShots = 1
SWEP.Primary.Force = 7

SWEP.Secondary.Ammo = "none"
SWEP.Secondary.Automatic = false

SWEP.enableKnife = true

SWEP.DeploySequence = "draw"
SWEP.SequencesHands = { "idle", "idlejudo", "idlekickass" }
SWEP.SequencesKnife = { "idleknife", "idleknifeinspect" }

SWEP.clientRagdollRemoved = true

function SWEP:CustomDeploy()
	self.CurrentViewModel:SetPlaybackRate(1.0)

	if self.enableKnife == true then
		self.CurrentViewModel:SetSequence("knifedraw")
		self:setBodyGroups("11")
		self:SetHoldType( "melee" )
		self:SetNW2Bool("enableKnife", self.enableKnife)
	else
		self.CurrentViewModel:SetSequence("ready")
		self:setBodyGroups("00")
		self:SetHoldType( "fist" )
		self:SetNW2Bool("enableKnife", self.enableKnife)
	end
	
	if game.SinglePlayer() then
		local function FadeOutAndRemoveEntity(entity, duration, delay)
			if not IsValid(entity) then return end
		
			entity:SetRenderMode(RENDERMODE_TRANSCOLOR)
			entity:SetColor(Color(255, 255, 255, 255))  

			local startTime = CurTime() + delay
			timer.Create("FadeOutTimer_" .. entity:EntIndex(), 0.1, 0, function()
				local elapsedTime = CurTime() - startTime
				local alpha = math.max(255 - (255 * (elapsedTime / duration)), 0) 
		
				entity:SetColor(Color(255, 255, 255, alpha))
		
				if alpha <= 0 then
					SafeRemoveEntity(entity)
					timer.Remove("FadeOutTimer_" .. entity:EntIndex())  
				end
			end)
		end

		if SERVER then
			util.AddNetworkString("clientRagdollRemoved" .. self:GetCreationID())
		end

		hook.Add("OnNPCKilled", "CreateRagdollAndApplyForce" .. self:GetCreationID(), function(npc, attacker, inflictor)
			if attacker ~= self.Owner then return end
			if self.enableKnife then return end

			self.clientRagdollRemoved = false 

			net.Start("clientRagdollRemoved" .. self:GetCreationID())
			net.WriteBool(self.clientRagdollRemoved)
			net.Send(self.Owner)
			
			local ragdoll = ents.Create("prop_ragdoll")

			ragdoll:SetModel(npc:GetModel())  
			ragdoll:SetPos(npc:GetPos()) 
			ragdoll:SetAngles(npc:GetAngles()) 
			ragdoll:SetCollisionGroup(COLLISION_GROUP_WORLD)

			ragdoll:Spawn()
			ragdoll:Activate()  
		
			local phys = ragdoll:GetPhysicsObject()
			
			timer.Simple(0.0, function()
				if IsValid(phys) then
					local f = 30000
					phys:ApplyForceCenter(self:GetOwner():GetForward() * f + self:GetOwner():GetUp() * f) 
				end
			end)

			FadeOutAndRemoveEntity(ragdoll, 3, 3)
		end)
	end
end

function SWEP:CustomHolster()
	timer.Remove("knifedraw" .. self:GetCreationID())
	timer.Remove("allowKnife" .. self:GetCreationID())
	timer.Remove("ready" .. self:GetCreationID())
	timer.Remove("allowPunch" .. self:GetCreationID())

	hook.Remove("OnNPCKilled", "CreateRagdollAndApplyForce" .. self:GetCreationID())
end

function SWEP:DoImpactEffect(tr, dmgtype)
	if not tr.Entity:IsValid() then return true end

    local matType = tr.MatType
	local effectdata = EffectData()
	effectdata:SetOrigin(tr.HitPos)

    if matType == MAT_FLESH then
        util.Effect("BloodImpact", effectdata)
	else
		if self.enableKnife then return true end
		util.Effect( "inflator_magic", effectdata )	
	end
	
	return true
end

function SWEP:MeleeAttack(damage, force, viewPunch, hitSound, missSound, hitDelay, missDelay, doShake, numBullets)
	local endPos = self.Owner:GetShootPos() + self.Owner:GetForward() * 64
	local trLine = util.TraceLine({
		start = self.Owner:GetShootPos(),
		endpos = endPos,
		filter = self.Owner
	})

	local ent = self:coneTrace(self.Owner, 16, 16, 64, Color(255, 0, 0))
    local hit = false

	if ent and ent:IsValid() and ent ~= game.GetWorld() then
        hit = true
		if SERVER then 
			ent:TakeDamage(damage, self.Owner, self)
            
            -- Some impact effect?
            self.Owner:FireBullets({
                Num = numBullets or 1,
                Src = self.Owner:GetShootPos(),
                Dir = self.Owner:GetAimVector(),
                Spread = Vector(0.1, 0.1),
                Tracer = 0,
                Force = force,
                Damage = GunmanShared.WeaponDamage.Knife.Knife,
                AmmoType = "none"
            })
		end
	end

    if SERVER then
        if hit or trLine.Fraction < 1 then
             self.Owner:EmitSound(hitSound)
             if doShake then util.ScreenShake(self:GetPos(), 10, 20, 0.1, 64) end
             self.Owner:SetViewPunchVelocity(viewPunch)
             return hitDelay
        else
             self.Owner:EmitSound(missSound)
             self.Owner:SetViewPunchVelocity(viewPunch)
             return missDelay
        end
    end
    
    return hitDelay 
end

function SWEP:PrimaryAttack()
	self.isFiring = true

	timer.Simple(2, function() 
		self.isFiring = false
	end)
	
	self.Owner:SetAnimation(PLAYER_ATTACK1)

	local swingDelay = 0.2

	if self.enableKnife == false then 
		if SERVER then 
			self.MeleeFlipFlop = not self.MeleeFlipFlop

			if self.MeleeFlipFlop then
				self:playSequence("leftpunch")
			else
				self:playSequence("rightpunch")
			end
		end

		swingDelay = self:MeleeAttack(
			10, 30, Angle(-30, 0, 0), "gunman_rightpunch", "gunman_hands_miss", 0.2, 0.4, true, 1
		)
	else
		if SERVER then 
			local choose = math.random(1, 2)
			if choose == 1 then
				self:playSequence("knifeattack1")
			else
				self:playSequence("knifeattack2")
			end
		end

		swingDelay = self:MeleeAttack(
			30, 2, Angle(-30, 30, 0), "gunman_hands_knifeattack2", "gunman_hands_knifeattack1", 0.5, 0.5, false, 4
		)
	end
	
	self:SetNextPrimaryFire(CurTime() + swingDelay)
end

function SWEP:SecondaryAttack()
	self.enableKnife = not self.enableKnife 

	self.isFiring = true

	local delayPrimaryFire = 2

	if self.enableKnife == true then 
		delayPrimaryFire = 1.6
		self:playSequence("holster")

		timer.Create("knifedraw" .. self:GetCreationID(), self:sequenceLength("knifeholster"), 1, function() 
			self:playSequence("knifedraw")
			self.Owner:EmitSound("gunman_hands_knifedraw")
			self:setBodyGroups("11")
			self:SetHoldType( "melee" )

			timer.Create("allowKnife" .. self:GetCreationID(), self:sequenceLength("knifedraw"), 1, function() 
				self.isFiring = false
			end)
		end)
	else
		delayPrimaryFire = 2
		self:playSequence("knifeholster")
		self.Owner:EmitSound("gunman_hands_knifeholster")

		timer.Create("ready" .. self:GetCreationID(), self:sequenceLength("knifeholster"), 1, function() 
			self:playSequence("ready")
			self:setBodyGroups("00")
			self:SetHoldType( "fist" )

			timer.Create("allowPunch" .. self:GetCreationID(), self:sequenceLength("ready"), 1, function() 
				self.isFiring = false
			end)
		end)
	end

	self:SetNextPrimaryFire(CurTime() + delayPrimaryFire)
	self:SetNextSecondaryFire(CurTime() + 1)	
end

function SWEP:CustomThink()
	self:SetNW2Bool("enableKnife", self.enableKnife)

	if CLIENT then
		net.Receive("clientRagdollRemoved" .. self:GetCreationID(), function()
			self.clientRagdollRemoved = net.ReadBool()
		end)
	end

	if not self.clientRagdollRemoved then 
		local position = self.Owner:GetPos() 
		local radius = 320      

		local entitiesInSphere = ents.FindInSphere(position, radius)

		if #entitiesInSphere > 0 then
			for _, ent in pairs(entitiesInSphere) do
				if CLIENT and IsValid(ent) then 
					if ent:GetClass() == "class C_ClientRagdoll" then
						ent:Remove()
					end
				end
			end
		end	

		self.clientRagdollRemoved = true
	end
end

function SWEP:GetIdleSequences()
    if self.enableKnife then
        return self.SequencesKnife
    else
        return self.SequencesHands
    end
end

function SWEP:PlayIdleSound(seqName)
    if seqName == "idlekickass" then
        self.Owner:EmitSound("gunman_hands_idlekickass")
    end
end




