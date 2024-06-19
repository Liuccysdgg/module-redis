-- redis_conn.lua
local redis_conn = {}
redis_conn.__index = redis_conn

--[[
    创建一个新的 fw_redis_conn 对象
    @return 返回一个新的 fw_redis_conn 对象
]]
function redis_conn.new(__module)
    local instance = setmetatable({}, redis_conn)
    instance.module = __module
    return instance
end

--[[
    执行 Redis 命令
    @param cmd Redis 命令字符串
    @return 返回命令执行结果
]]
function redis_conn:command(cmd)
    return self.module:command(cmd)
end

return redis_conn
