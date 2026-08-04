local io = require("io")

local a = 332
print(a)

io.write("Hi: ")
io.flush()
local user_input = io.read()
local file = io.open("super.lua", "w")

if file then
  file:write("a = \"Hello!\"")
  file:write("\nprint(a)")
  file:close()
else
  print("Could NOT open file")
end

print("Got: " .. user_input .. " as input")
