#pragma once
#include "base/define.h"
#include "util/queue.hpp"
#include "sol/sol.hpp"
#ifdef _WIN32
#include "redis.h"
#else
#include <hiredis/hiredis.h>
#endif
#include "basemodule.h"
namespace module
{
	void redis_regist(sol::state* lua);
	class redis;
	/// <summary>
	/// 连接池
	/// </summary>
	class redis_pool:public module::base
	{
	public:
		redis_pool();
		~redis_pool();
		/// <summary>
		/// 创建
		/// </summary>
		/// <param name="address"></param>
		/// <param name="port"></param>
		/// <param name="password"></param>
		/// <param name="max_size"></param>
		void start(const std::string& address,ushort port,const std::string& password,int max_size);
		/// <summary>
		/// 关闭
		/// </summary>
		void close();
		/// <summary>
		/// 获取连接
		/// </summary>
		/// <returns></returns>
		std::shared_ptr<module::redis> get();
		/// <summary>
		/// 取弹出连接数
		/// </summary>
		/// <returns></returns>
		int pop_size();

		virtual void regist_global(const char* name, sol::state* lua) override;
		virtual void delete_global() { delete this; }


		friend class module::redis;
	private:
		/// <summary>
		/// 重新获取
		/// </summary>
		/// <param name="ctx">归还CTX</param>
		/// <returns></returns>
		redisContext* reget(redisContext* ctx);
		/// <summary>
		/// 处理回复
		/// </summary>
		/// <param name="ct"></param>
		/// <param name="reply"></param>
		/// <returns></returns>
		sol::object reply(sol::this_state* ts,redisContext* ct,redisReply* reply);
		/// <summary>
		/// 归还
		/// </summary>
		/// <param name="ctx"></param>
		void recover(redisContext* ctx);
	private:
		// 池队列
		ylib::queue<redisContext*> m_queue;
		// 地址
		std::string m_address;
		// 端口
		ushort m_port = 0;
		// 密码
		std::string m_password;
		// 最大数量
		int m_max_size = 0;
		// 已弹出
		int m_pop_size = 0;
		// 锁
		std::mutex m_mutex;
		// 已关闭
		bool m_closed = true;
	};
	/// <summary>
	/// REDIS连接
	/// </summary>
	class redis{
	public:
		redis(redisContext* context,redis_pool* pool);
		~redis();
		/// <summary>
		/// 执行命令
		/// </summary>
		/// <param name="format"></param>
		/// <param name=""></param>
		/// <returns></returns>
		sol::object command(const std::string& cmd, sol::this_state ts);
	private:
		// 连接
		redisContext* m_context = nullptr;
		// 池
		redis_pool* m_pool = nullptr;
	};
}


