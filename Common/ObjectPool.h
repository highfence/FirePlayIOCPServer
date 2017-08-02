#pragma once
#include <vector>
#include <memory>
#include <deque>
#include <mutex>

namespace FirePlayCommon
{
	/*
		ObjectPool
		��Ƽ������ ȯ�濡�� ���ư� �� �ֵ��� § ObjectPoolŬ����.
		Session�� ������ ����ϱ� ���Ͽ� �켱 �����ϰ� ����.
	*/
	template<class T>
	class ObjectPool
	{
	public:

		ObjectPool() {};
		~ObjectPool() { Release(); };

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

		bool _isInitialized = false;

		std::mutex _poolMutex;
		std::vector<std::unique_ptr<T>> _pool;
		std::deque<int> _poolIndex;
	};

	// ������Ʈ Ǯ �������� ũ�⸦ �̿��Ͽ� �ʱ�ȭ�� ���־�� �Ѵ�.
	template<class T>
	inline void ObjectPool<T>::Init(const int poolSize)
	{
		_pool.reserve(poolSize);

		for (int i = 0; i < poolSize; ++i)
		{
			_pool.emplace_back(std::make_unique<T>());
			_poolIndex.push_back(i);
		}

		_isInitialized = true;
	}

	template<class T>
	inline void ObjectPool<T>::Release()
	{
		_isInitialized = false;

		std::lock_guard<std::mutex> releaseLock(_poolMutex);
		_pool.clear();
		_pool.shrink_to_fit();

		_poolIndex.clear();
		_poolIndex.shrink_to_fit();
	}

	template<class T>
	inline int ObjectPool<T>::GetTag()
	{
		if (!_isInitialized) return -1;

		std::lock_guard<std::mutex> tagLock(_poolMutex);
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
		if (!_isInitialized) return;

		std::lock_guard<std::mutex> tagLock(_poolMutex);
		_poolIndex.push_back(tag);
	}

	template<class T>
	inline int ObjectPool<T>::GetSize()
	{
		if (!_isInitialized) return -1;

		std::lock_guard<std::mutex> sizeLock(_poolMutex);
		return static_cast<int>(_pool.size());
	}

	template<class T>
	inline T & ObjectPool<T>::begin()
	{
		if (!_isInitialized) return -1;

		std::lock_guard<std::mutex> poolLock(_poolMutex);
		return _pool.begin();
	}

	template<class T>
	inline T & ObjectPool<T>::end()
	{
		if (!_isInitialized) return -1;

		std::lock_guard<std::mutex> poolLock(_poolMutex);
		return _pool.end();
	}

	template<class T>
	inline T & ObjectPool<T>::operator[](const int idx)
	{
		std::lock_guard<std::mutex> poolLock(_poolMutex);
		return *_pool[idx];
	}

	template<class T>
	inline bool ObjectPool<T>::IsEmpty()
	{
		if (!_isInitialized) return true;

		std::lock_guard<std::mutex> poolLock(_poolMutex);
		return _pool.empty();
	}
}