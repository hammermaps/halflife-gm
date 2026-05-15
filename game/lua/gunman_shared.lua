AddCSLuaFile()

local gunmanData = include("gunman_data.lua")
local GunmanShared = {}

GunmanShared.WeaponDamage = gunmanData.WeaponDamage
GunmanShared.EntityDamage = gunmanData.EntityDamage
GunmanShared.DefaultWeaponSettings = gunmanData.DefaultWeaponSettings
GunmanShared.WeaponAmmoUsage = gunmanData.WeaponAmmoUsage

function GunmanShared.EntityCreate(parent, entName, pos, ang, model, setupCallback)
    if not SERVER then return end
    
    local ent = ents.Create(entName)
    if not IsValid(ent) then 
        print("GunmanShared.EntityCreate: Failed to create entity " .. tostring(entName))
        return 
    end

    if model and model ~= "" then
        ent:SetModel(model)
    end

    if ang and ang ~= 0 then
        ent:SetAngles(ang)
    end

    if IsValid(parent) and IsValid(parent:GetOwner()) then
        ent:SetOwner(parent:GetOwner())
    end

    ent:SetPos(pos)

    if setupCallback then
        setupCallback(ent)
    end

    ent:Spawn()
    ent:Activate()

    return ent
end

function GunmanShared.GetSurfaceInfo(ent)
    local entityPos = ent:GetPos()

    local traces = {
        {start = entityPos, endpos = entityPos + Vector(0, 0, -64)}, -- Down
        {start = entityPos, endpos = entityPos + Vector(0, 0, 64)},  -- Up
        {start = entityPos, endpos = entityPos + Vector(-64, 0, 0)}, -- Left
        {start = entityPos, endpos = entityPos + Vector(64, 0, 0)},  -- Right
        {start = entityPos, endpos = entityPos + Vector(0, 64, 0)},  -- Front
        {start = entityPos, endpos = entityPos + Vector(0, -64, 0)}  -- Back
    }

    for _, t in ipairs(traces) do
        t.filter = ent
        local trace = util.TraceLine(t)
        if trace.Hit and (trace.Fraction * 64) <= 32 then
            return trace.HitPos, trace.HitNormal
        end
    end

    return nil, nil
end

function GunmanShared.DoSegmentedBeam(startPos, endPos, width, segments, noiseAmount, endPosNoiseAmount, beamMaterial, color)
    if not CLIENT then return end

    local points = {}
    points[1] = startPos

    local totalDistance = (endPos - startPos):Length()
    local distanceClamped = math.Clamp(totalDistance * 0.001, 0, 2)

    local randomEndPos = VectorRand(-endPosNoiseAmount, endPosNoiseAmount) * distanceClamped 
    local actualEndPos = endPos + randomEndPos

    local shootDir = (actualEndPos - startPos):GetNormalized()
    local segmentDistance = totalDistance / segments

    for i = 2, segments do
        local prevPoint = points[i - 1]
        local randomOffset = VectorRand(-noiseAmount, noiseAmount) * distanceClamped 

        points[i] = prevPoint + randomOffset + shootDir * segmentDistance
    end

    points[segments + 1] = actualEndPos 

    render.SetMaterial(Material(beamMaterial))
    render.StartBeam(segments + 1)

    for i = 1, segments + 1 do
        render.AddBeam(points[i], width, CurTime() - i * (totalDistance / 1000), color)
    end

    render.EndBeam()    
end

function GunmanShared.ViewModelColorCorrection(swep)
    if not CLIENT then return end
    if not IsValid(swep) or not IsValid(swep:GetOwner()) then return end
    
    local viewmodel = swep:GetOwner():GetViewModel()
    if not IsValid(viewmodel) then return end

    local lightingColor = render.GetLightColor(swep:GetPos()) 
    viewmodel:SetColor(Color( 
        (lightingColor.x ^ 0.25) * 255, 
        (lightingColor.y ^ 0.25) * 255, 
        (lightingColor.z ^ 0.25) * 255 
    ))
end

function GunmanShared.GaussianRandom(mean, stddev)
    local u1 = math.Rand(0, 1)
    local u2 = math.Rand(0, 1)

    local z0 = math.sqrt(-2 * math.log(u1)) * math.cos(2 * math.pi * u2)

    return mean + z0 * stddev
end

function GunmanShared.CustomBellCurveRandom(numTable, mean, stddev)
    if not numTable or #numTable == 0 then return 0 end
    local gaussValue = GunmanShared.GaussianRandom(mean, stddev)
    local clampedValue = math.max(1, math.min(#numTable, math.floor(gaussValue)))
    return numTable[clampedValue]
end

function GunmanShared.ClampedTracerHitPos(parent, maxRange, randomXY)
    if not IsValid(parent) or not IsValid(parent:GetOwner()) then return Vector(0, 0, 0) end
    
    randomXY = randomXY or 0
    local owner = parent:GetOwner()
    local eyePos = owner:EyePos()
    local tr = owner:GetEyeTrace()
    local hitPos = tr.HitPos
    
    if randomXY ~= 0 then
        hitPos = hitPos + Vector(0, math.random(-randomXY, randomXY), math.random(-randomXY, randomXY))
    end

    local dir = (hitPos - eyePos):GetNormalized() 
    local distance = (hitPos - eyePos):Length()
    
    if distance > maxRange then
        hitPos = eyePos + dir * maxRange
    end
    
    return hitPos
end

function GunmanShared.BlastDamage(attacker, inflictor, pos, radius, damage, type)
    local dmg = DamageInfo()
    dmg:SetAttacker(IsValid(attacker) and attacker or game.GetWorld())
    dmg:SetInflictor(IsValid(inflictor) and inflictor or game.GetWorld())
    dmg:SetDamage(damage)
    dmg:SetDamagePosition(pos)

    dmg:SetDamageType(type)

    util.BlastDamageInfo(dmg, pos, radius)
end

return GunmanShared
