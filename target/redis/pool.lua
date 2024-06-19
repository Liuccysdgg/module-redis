local conn = require("redis.conn")

local redis_pool = {}
redis_pool.__index = redis_pool

--[[
    创建一个新的 fw_redis_pool 对象
    @return 返回一个新的 fw_redis_pool 对象
]]
function redis_pool.new()
    local instance = setmetatable({}, redis_pool)
    instance.module = fw_redis_pool.new()
    return instance
end

--[[
    启动 Redis 连接池
    @param address 地址
    @param port 端口号
    @param password 密码
    @param max_size 最大连接数
]]
function redis_pool:start(address, port, password, max_size)
    self.module:start(address, port, password, max_size)
end

--[[
    关闭 Redis 连接池
]]
function redis_pool:close()
    self.module:close()
end

--[[
    获取 Redis 连接
    @return 返回一个 redis 连接对象
]]
function redis_pool:get()
    return conn.new(self.module:get())
end

--[[
    获取弹出连接数
    @return 返回弹出连接数
]]
function redis_pool:pop_size()
    return self.module:pop_size()
end

return redis_pool
