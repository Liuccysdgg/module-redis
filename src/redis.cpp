#include "redis.h"
#include "dll_interface.h"
extern "C" {
#ifdef _WIN32
    DLL_EXPORT
#endif
        int fastweb_module_regist(void* sol2, void* lua)
    {
        sol::state* state = static_cast<sol::state*>(sol2);
        module::redis_regist(state);
        return 0;
    }
}
module::redis_pool::redis_pool()
{

}

module::redis_pool::~redis_pool()
{
}

void module::redis_pool::start(const std::string& address, ushort port, const std::string& password, int max_size)
{
    close();
    
    std::unique_lock<std::mutex> uni(m_mutex);

    m_closed = false;
    m_address = address;
    m_port = port;
    m_password = password;
    m_max_size = max_size;
}

void module::redis_pool::close()
{
    std::unique_lock<std::mutex> uni(m_mutex);
    redisContext* ctx = nullptr;
    while (m_queue.pop(ctx))
        redisFree(ctx);
    m_pop_size = 0;
    m_closed = true;
}

std::shared_ptr<module::redis> module::redis_pool::get()
{
    if (m_closed)
    {
        throw ylib::exception("connection pool is closed");
    }
    redisContext* ctx = nullptr;
    if (m_queue.pop(ctx))
    {
        m_pop_size++;
        return std::make_shared<module::redis>(ctx, this);
    }
    if (m_pop_size >= m_max_size)
        throw ylib::exception("connection pool releases more connections than the maximum number");

    ctx = reget(nullptr);
    
    m_pop_size++;
    return std::make_shared<module::redis>(ctx, this);
}

int module::redis_pool::pop_size()
{
    return m_pop_size;
}

void module::redis_pool::regist_global(const char* name, sol::state* lua)
{
    lua->registry()[name] = this;
    (*lua)[name] = this;
}

redisContext* module::redis_pool::reget(redisContext* ctx)
{
    if (ctx != nullptr)
    {
        redisFree(ctx);
        ctx = nullptr;
    }

    redisContext* context = redisConnect(m_address.c_str(), m_port);
    if (context == NULL || context->err) {
        std::string exception;
        if (context) {
            exception = "connection error: " + std::string(context->errstr);
            redisFree(context);
        }
        else
            exception = "Connection error: can't allocate redis context";
        throw ylib::exception(exception);
    }

    if (m_password != "")
    {
        try
        {
            reply(nullptr, context, (redisReply*)redisCommand(context, "AUTH %s", m_password.c_str()));
        }
        catch (const ylib::exception& e)
        {
            redisFree(context);
            throw e;
        }
    }
    return context;
}

sol::object module::redis_pool::reply(sol::this_state* ts,redisContext* ctx, redisReply* reply)
{
    if (reply == NULL) {
        if (ctx->err)
            throw ylib::exception("connection error: " + std::string(ctx->errstr));
        else
            throw ylib::exception("Unknown error: reply is NULL but context has no error\n");
    }

    sol::object result;
    try
    {
        switch (reply->type) {
        case REDIS_REPLY_ERROR:
            throw ylib::exception("redis error: " + std::string(reply->str));
            break;
        case REDIS_REPLY_STATUS:
        case REDIS_REPLY_STRING:
            if (ts != nullptr)
                result = sol::make_object(*ts, reply->str);
            break;
        case REDIS_REPLY_INTEGER:
            if (ts != nullptr)
                result = sol::make_object(*ts, reply->integer);
            break;
        case REDIS_REPLY_NIL:
            if (ts != nullptr)
                result = sol::make_object(*ts, sol::nil);
            break;
        case REDIS_REPLY_ARRAY:
            if (ts != nullptr)
            {
                sol::state_view lua(*ts);
                sol::table table = lua.create_table();
                for (size_t i = 0; i < reply->elements; i++) {
                    table[i + 1] = this->reply(ts, ctx, reply->element[i]);
                }
                result = table;
            }
            break;
        default:
            throw ylib::exception("Unknown reply type: " + std::to_string(reply->type));
        }
    }
    catch (const std::exception& e)
    {
        freeReplyObject(reply);
        throw e;
    }
    freeReplyObject(reply);
    return result;
}

void module::redis_pool::recover(redisContext* ctx)
{
    if (ctx == nullptr)
        return;
    if (m_closed)
    {
        redisFree(ctx);
        return;
    }
    m_queue.push(ctx);
    m_pop_size--;
}

module::redis::redis(redisContext* context, redis_pool* pool):m_context(context),m_pool(pool)
{

}

module::redis::~redis()
{
    m_pool->recover(m_context);
}

sol::object module::redis::command(const std::string& cmd, sol::this_state ts)
{
    //return m_pool->reply(&ts,m_context,(redisReply*)redisCommand(m_context, cmd.c_str()));
    auto reply = (redisReply*)redisCommand(m_context, cmd.c_str());
    if (reply == NULL) {
        if (m_context->err)
        {
            m_context = m_pool->reget(m_context);

            reply = (redisReply*)redisCommand(m_context, cmd.c_str());
        }
        else
            throw ylib::exception("Unknown error: reply is NULL but context has no error\n");
    }
    return m_pool->reply(&ts,m_context,reply);


}

void module::redis_regist(sol::state* lua)
{
    lua->new_usertype<module::redis_pool>("redis_pool",
        "close", &module::redis_pool::close,
        "start", &module::redis_pool::start,
        "get", &module::redis_pool::get,
        "pop_size", &module::redis_pool::pop_size,
        "self", &module::redis_pool::self
    );
    lua->new_usertype<module::redis>("redis_conn",
        "command", &module::redis::command
    );
}
