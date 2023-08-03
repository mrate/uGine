local res = ugine.engine:getResources()
local id = res:findResource("editor/Box.umodel")
ugine:info(id)
if not id:isNull() then
    local model = res:getModel(id)
    ugine:info("Resource type: ", model:get().type.name)
    ugine:info("Ready: ", model:get().ready)

    local world = ugine.editor:activeWorld()
    local obj = world:createObject("Mesh")
    local mesh = obj:createMeshComponent()
    mesh:setModel(model)
end

-- anim

local res = ugine.engine:getResources()
local id = res:findResource("new/CesiumMan_animation#0.uanim")
ugine:info(id:toString())
if not id:isNull() then
    local anim = res:getAnimation(id)
    ugine:info("Resource type: ", anim:get().type.name)
    ugine:info("Ready: ", anim:get().ready)

    local world = ugine.editor:activeWorld()
    local obj = world:createObject("Mesh")
    local ctrl = obj:createAnimationController()
    ctrl.animation = anim
    obj:updateAnimationController()
end

--

local res = ugine.engine:getResources()

local modelId = res:findResource("editor/Box.umodel")
local model = res:getModel(modelId)

local materialId = res:findResource("new/StandardMaterial.umat")
local material = res:getMaterial(materialId)

local world = ugine.editor:activeWorld()
local obj = world:createObject("New object")

mesh=obj:createMeshComponent()
mesh:setModel(model)
mesh:setMaterial(0, material)
mesh.instanced = true
mesh.instanceCount = 100

for i=0,mesh.instanceCount - 1,1
do
    local trans=mesh:getInstanceTransformation(i)
    trans.position = ugine.Vec3( i % 10, math.floor(i / 10), 0)
    trans.scale = ugine.Vec3(0.9, 0.9, 0.9 + 0.2 * math.sin(i))
    mesh:setInstanceTransformation(i, trans)
end

