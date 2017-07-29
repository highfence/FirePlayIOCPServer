#pragma once
#include <vector>
#include <memory>
#include <deque>
#include <mutex>

namespace FirePlayCommon
{
	/*
		ObjectPool
		멀티스레드 환경에서 돌아갈 수 있도록 간단하게 짠 ObjectPool클래스.
		Session의 정보를 기록하기 위하여 우선 제작.
	*/
	template<class T>
	class ObjectPool
	{
	public:

		ObjectPool() {};
		~ObjectPool() {};

		void Init(const int poolSize);
		void Release();

		int GetTag();
		void ReleaseTag(const int tag);

		int GetSize();
		bool IsEmpty();

		T& begin();
		T& end();
		T& operator[](const int idx);

	private:

		std::mutex _poolMutex;
		std::vector<std::unique_ptr<T>> _pool;

		std::mutex _poolIndexMutex;
		std::deque<int> _poolIndex;
	};

	template<class T>
	inline void ObjectPool<T>::Init(const int poolSize)
	{
		_pool.reserve(poolSize);

		for (int i = 0; i < poolSize; ++i)
		{
			_pool.emplace_back(std::make_unique<T>());
			_poolIndex.push_back(i);
		}
	}

	template<class T>
	inline void ObjectPool<T>::Release()
	{
		std::lock_guard<std::mutex> _idxLock(_poolIndexMutex);
		_pool.clear();
		_pool.shrink_to_fit();

		std::lock_guard<std::mutex> _idxLock(_poolMutex);
		_poolIndex.clear();
		_poolIndex.shrink_to_fit();
	}

	template<class T>
	inline int ObjectPool<T>::GetTag()
	{
		std::lock_guard<std::mutex> _idxLock(_poolIndexMutex);
		if (_poolIndex.empty())
		{
			return -1;
		}

		auto returnTag = _poolIndex.front();
		_poolIndex.pop_front();

		return returnTag;
	}

	template<class T>
	inline void ObjectPool<T>::ReleaseTag(const int tag)
	{
		std::lock_guard<std::mutex> _idxLock(_poolIndexMutex);
		_poolIndex.push_back(tag);
	}

	template<class T>
	inline int ObjectPool<T>::GetSize()
	{
		std::lock_guard<std::mutex> _poolLock(_poolMutex);
		return _pool.size();
	}

	template<class T>
	inline T & ObjectPool<T>::begin()
	{
		std::lock_guard<std::mutex> _poolLock(_poolMutex);
		return _pool.begin();
	}

	template<class T>
	inline T & ObjectPool<T>::end()
	{
		std::lock_guard<std::mutex> _poolLock(_poolMutex);
		return _pool.end();
	}

	template<class T>
	inline T & ObjectPool<T>::operator[](const int idx)
	{
		std::lock_guard<std::mutex> _poolLock(_poolMutex);
		return *_pool[idx];
	}

	template<class T>
	inline bool ObjectPool<T>::IsEmpty()
	{
		std::lock_guard<std::mutex> _poolLock(_poolMutex);
		return _pool.empty();
	}
}