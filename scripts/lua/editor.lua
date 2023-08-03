--
world=ugine.editor:activeWorld()
light=world:find("Direction light")
if light.isValid then
  ugine:info("Selecting: ", light.name)
  ugine.editor:select(light)

  light:createMesh()
end


-- instance
world = ugine.editor:activeWorld()
obj = ugine.editor:selected()
if obj:isValid() then
    if not obj:hasMesh() then
        obj:createMesh()
    end

    mesh = obj:getMesh()
    mesh:setInstanced(true)
    mesh:setInstanceCount(100)
    
    for i=0,99,1
    do
        trans=mesh:getInstanceTransformation(i)
        trans.position = ugine.Vec3( i % 10, math.floor(i / 10), 0)
        trans.scale = ugine.Vec3(0.9, 0.9, 0.9 + 0.2 * math.sin(i))
        mesh:setInstanceTransformation(i, trans)
    end
end


---

local world=ugine.editor:activeWorld()
local lights=world:findAll("Direction light")

local light = lights[1]
ugine:info("First light: ", light.name)

for v,k in pairs(lights) do
  ugine:info(v, " - ", k.name)
end