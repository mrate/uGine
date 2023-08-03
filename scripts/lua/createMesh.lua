local world = ugine.editor:activeWorld()
local obj = world:createObject("ent")
mesh=obj:createMesh()
--local obj = ugine.editor:selected()
--local mesh = obj:getMesh()
mesh.instanced = true
mesh.instanceCount = 100

for i=0,99,1
do
    local trans=mesh:getInstanceTransformation(i)
    trans.position = ugine.Vec3( i % 10, math.floor(i / 10), 0)
    trans.scale = ugine.Vec3(0.9, 0.9, 0.9 + 0.2 * math.sin(i))
    mesh:setInstanceTransformation(i, trans)
end



wm = ugine.engine:getWorldManager()
world = wm:getWorld(2)
obj=world:find('ent')
if obj.isValid then
    ugine:info("Found ent!")
    obj:destroyMesh()
end