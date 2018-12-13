/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * DataBuffer.h Simple buffer that either references external memory or
 * an internal vector.
 *
 *  Created on: Dec 13, 2018
 *      Author: Aron Helser aron.helser@kitware.com
 */

#ifndef ADIOS2_CORE_DATABUFFER_H_
#define ADIOS2_CORE_DATABUFFER_H_

#include <vector>
#include <memory>

template<typename T>
class DataBuffer
{
public:
	DataBuffer() : m_Ptr(nullptr) { }
	DataBuffer(const T *ptr) : m_Ptr(ptr) { }

	DataBuffer(size_t count) : m_Buf(std::make_shared<std::vector<T>>(count)), m_Ptr(m_Buf.data()) { }

	const T* Ptr() const { return m_Ptr; }

private:
	std::shared_ptr<std::vector<T>> m_Buf;
	const T* m_Ptr;
};

#endif
