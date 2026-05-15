EFFECT.material = "gunman/sprites/muzzleflash3"

function EFFECT:Init(data)
    self.Owner = data:GetEntity()
    self.scale = data:GetScale() or 1
    self.bone = data:GetFlags()

    self.startTime = CurTime()       
    self.lifeTime = 0.05
end

function EFFECT:Think()
    return (CurTime() - self.startTime) < self.lifeTime
end

function EFFECT:Render()
    if self.Owner != LocalPlayer() or render.GetRenderTarget() ~= nil then 

        local weapon = self.Owner:GetActiveWeapon()
        local attachment = weapon:GetAttachment(weapon:LookupAttachment("muzzle"))

        if attachment then
            render.SetMaterial(Material(self.material))
            render.DrawSprite(attachment.Pos, 16 * self.scale, 16 * self.scale, Color(255, 255, 255, 200))
        end
    else
        local viewPos = self.Owner:GetViewModel():GetBonePosition(self.bone)
        if viewPos == self.Owner:GetViewModel():GetPos() then
            viewPos = self.Owner:GetViewModel():GetBoneMatrix(0):GetTranslation()
        end

        render.SetMaterial(Material(self.material))
        render.DrawSprite(viewPos, 32 * self.scale, 32 * self.scale, Color(255, 255, 255, 200))
    end
end