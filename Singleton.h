#pragma once

#ifndef SINGLETON_H
#define SINGLETON_H

#include <memory>
#include <mutex>

template<typename T>
class Singleton{
private:
	static void InitSingleton() {
		instance = std::shared_ptr<T>(new T, [](T* ptr) {
			delete ptr;
		});
	}

	static std::shared_ptr<T> instance;
	static std::once_flag initFlag;
protected:
	Singleton() {
		System::Debug::Log(System::Debug::ConvertString(L"[Singleton] : "));
	}
	~Singleton() {
		System::Debug::Log(System::Debug::ConvertString(L"[Singleton] : "));
	}

	Singleton(const Singleton&) = delete;
	Singleton(const Singleton&&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	Singleton& operator=(const Singleton&&) = delete;

public:
	static std::shared_ptr<T> getInstance() {
		std::call_once(initFlag, &Singleton::InitSingleton);
		return instance;
	}
};

template<typename T>
std::shared_ptr<T> Singleton<T>::instance = nullptr;
template<typename T>
std::once_flag Singleton<T>::initFlag;

#endif

