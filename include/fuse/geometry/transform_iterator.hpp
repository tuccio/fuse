#pragma once

#include <fuse/allocators.hpp>
#include <fuse/math.hpp>

#include <iterator>

namespace fuse
{

	template <typename Iterator>
	class alignas(16) transform_iterator :
	{

		transform_iterator(void) = default;
		transform_iterator(const transform_iterator &) = default;
		transform_iterator(transform_iterator &&) = default;

		transform_iterator(Iterator it, const XMMATRIX & transform) :
			m_iterator(it), m_transform(transform) { }
	
		transform_iterator & operator=(const transform_iterator &) = default;

		transform_iterator & operator++()
		{
			return ++m_iterator;
		}

		transform_iterator operator++(int)
		{
			return m_iterator++;
		}

		value_type operator*(void) const
		{
			return XMVector3Transform(to_vector(*m_iterator), m_transform);
		}

		/*pointer operator->(void) const
		{
			return m_iterator;
		}*/

		bool operator==(const transform_iterator & b)
		{
			return m_iterator == b.m_iterator;
		}

		bool operator!=(const transform_iterator & b)
		{
			return m_iterator != b.m_iterator;
		}

	private:

		XMMATRIX m_transform;
		Iterator m_iterator;

	};

	template <typename Iterator>
	transform_iterator<Iterator> make_transform_iterator(Iterator it, const XMMATRIX & transform)
	{
		return transform_iterator<Iterator>(it, transform);
	}

}