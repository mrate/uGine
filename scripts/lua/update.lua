local script = {}

script.params = {
    param1 = ugine.Vec3(1, 2, 3),
    param2 = 5,
    param3 = "name"
}

local lastTime = 0
local angle = 0

function script.attach(entity)
    ugine.debug("Attached")

    if not entity:hasMeshComponent() then
        entity:createMeshComponent()
    end

    local mesh = entity:getMeshComponent()

    local res = ugine.engine:getResources()
    local id = res:findResource("editor/Box.umodel")

    if not id:isNull() then
      local model = res:getModel(id)
      mesh:setModel(model)
    end
end

function script.detach(entity)
    ugine.debug("Detached")
end

function script.update(entity)
    angle = angle + ugine.engine.secondsDelta
    local t = entity.localTransform
    t.rotation = ugine.Quat.angleAxis(angle, ugine.Vec3(0, 1, 0.5))
    entity.localTransform = t

    local time = ugine.engine.frameSeconds
    if time - lastTime > 1 then
        ugine.debug("Update")
        lastTime = time
    end
end

return script
