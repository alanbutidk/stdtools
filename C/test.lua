local io = require("io")

local a = 332
print(a)

io.write("Hi: ")
io.flush()
local user_input = io.read()

print("Got: " .. user_input .. " as input")
