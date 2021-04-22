#pragma once


// Forward declarations
template<typename T>
class DeferredVector;

template<typename T>
class LiteVector;


template<typename T>
struct LiteVectorIterator {
	LiteVector<T>* parent;
	uint32_t current_idx;

	LiteVectorIterator() {};

	bool operator!=(LiteVectorIterator& iter_right)
	{
		return parent != iter_right.parent || current_idx != iter_right.current_idx;
	}

	T& get()
	{
		return *(parent->data + current_idx);
	}

	inline uint32_t index()
	{
		return current_idx;
	}

	void next()
	{
		current_idx++;
	}
};


template<typename T>
class LiteVector {
public:
	uint32_t size;
	uint32_t capacity;
	T* data;

	LiteVector()
	{
		size = 0;
		capacity = 0;
	}

	void resize(uint32_t new_size)
	{
		if (capacity == 0) {
			data = new T[new_size];
			capacity = new_size;
		}
		else if (capacity < new_size) {

			T* new_data = new T[new_size];

			std::memcpy(new_data, data, capacity * sizeof(T));

			delete[] data;

			data = new_data;
			capacity = new_size;
		}

		size = new_size;
	}

	void reserve(uint32_t new_capacity)
	{
		if (capacity == 0) {
			data = new T[new_capacity];
			capacity = new_capacity;
		}
		else if (new_capacity > capacity) {

			T* new_data = new T[new_capacity];

			std::memcpy(new_data, data, new_capacity * sizeof(T));

			delete[] data;

			data = new_data;
			capacity = new_capacity;

			// size unchanged
		}
	}

	void clear()
	{
		size = 0;
	}

	T& operator[](uint32_t idx)
	{
		return data[idx];
	}

	T& emplace_back()
	{
		this->resize(size + 1);
		return data[size - 1];
	}

	LiteVectorIterator<T> begin()
	{
		LiteVectorIterator<T> iter;
		iter.parent= this;
		iter.current_idx = 0;

		return iter;
	}

	LiteVectorIterator<T> end()
	{
		LiteVectorIterator<T> iter;
		iter.parent = this;
		iter.current_idx = size;

		return iter;
	}

	~LiteVector()
	{
		if (capacity) {
			delete[] data;
		}
	}
};


template<typename T>
struct DeferredVectorNode {
	bool is_deleted;
	T elem;
};


template<typename T>
class SparseVectorIterator {
public:
	DeferredVector<T>* _parent_vector;
	uint32_t _current_idx;

public:
	SparseVectorIterator() {};

	bool operator!=(SparseVectorIterator& iter_right)
	{
		return _parent_vector != iter_right._parent_vector || _current_idx != iter_right._current_idx;
	}

	T& get()
	{
		return _parent_vector->elems[_current_idx].elem;
	}

	inline uint32_t index()
	{
		return _current_idx;
	}

	void next()
	{
		LiteVector<DeferredVectorNode<T>>& elems = _parent_vector->elems;

		_current_idx++;

		while (true) {
			if (_current_idx > _parent_vector->_last_index) {
				return;  // reached the end
			}

			if (elems[_current_idx].is_deleted == false) {
				return;  // element found
			}

			_current_idx++;
		}
	}
};


// made specifically for holding primitive data
// deletions are cheap as they don't have to reallocate the whole vector
// preserves indexes
template<typename T>
class DeferredVector {
public:
	uint32_t _size;  // current used size without deleted elements

	// first element in vector may be deleted so keep track so you don't have to skip on each begin
	uint32_t _first_index;
	uint32_t _last_index;

	LiteVector<DeferredVectorNode<T>> elems;
	LiteVector<uint32_t> deleted;

public:
	DeferredVector()
	{
		_size = 0;
		_first_index = 0;
		_last_index = 0;
	}

	void resize(uint32_t new_size)
	{
		elems.resize(new_size);
		deleted.clear();

		_size = new_size;
		_first_index = 0;
		_last_index = new_size - 1;

		// clear deleted flags
		for (auto iter = elems.begin(); iter != elems.end(); iter.next()) {
			DeferredVectorNode<T>& node = iter.get();
			node.is_deleted = false;
		}
	}

	void reserve(uint32_t new_capacity)
	{
		elems.reserve(new_capacity);
	}

	//T* _tryReuseDeleted()
	//{
	//	for (auto iter = deleted.begin(); iter != deleted.end(); iter.next()) {

	//		uint32_t& deleted_idx = iter.get();
	//		if (deleted_idx != 0xFFFF'FFFF) {

	//			// mark node as available
	//			DeferredVectorNode<T>& recycled_node = elems[deleted_idx];
	//			recycled_node.is_deleted = false;

	//			// bounds update
	//			if (deleted_idx < _first_index) {
	//				_first_index = deleted_idx;
	//			}
	//			else if (deleted_idx > _last_index) {
	//				_last_index = deleted_idx;
	//			}

	//			// remove from deleted list
	//			deleted_idx = 0xFFFF'FFFF;

	//			return &recycled_node.elem;
	//		}
	//	}

	//	return nullptr;
	//}

	T& emplace(uint32_t& r_index)
	{
		// try reuse deleted
		for (auto iter = deleted.begin(); iter != deleted.end(); iter.next()) {

			uint32_t& deleted_idx = iter.get();
			if (deleted_idx != 0xFFFF'FFFF) {

				// mark node as available
				DeferredVectorNode<T>& recycled_node = elems[deleted_idx];
				recycled_node.is_deleted = false;

				// bounds update
				if (deleted_idx < _first_index) {
					_first_index = deleted_idx;
				}
				else if (deleted_idx > _last_index) {
					_last_index = deleted_idx;
				}

				// remove from deleted list
				deleted_idx = 0xFFFF'FFFF;

				_size++;

				return recycled_node.elem;
			}
		}

		_size++;

		r_index = elems.size;

		DeferredVectorNode<T>& new_node = elems.emplace_back();
		return new_node.elem;
	}

	void erase(uint32_t index)
	{
		DeferredVectorNode<T>& node = elems[index];
		if (node.is_deleted == false) {

			node.is_deleted = true;

			// seek forward new first index
			if (index == _first_index) {
				
				uint32_t i = index + 1;

				while (true) {
					if (i == _last_index || elems[i].is_deleted == false) {
						_first_index = i;
						break;
					}
					i++;
				}
			}
			// seek backward new last index
			else if (index == _last_index && index > 1) {
				
				uint32_t i = index - 1;

				while (true) {
					if (i == _first_index || elems[i].is_deleted == false) {
						_last_index = i;
						break;
					}
					i--;
				}
			}

			_size--;

			// add to delete list
			for (auto iter = deleted.begin(); iter != deleted.end(); iter.next()) {
				uint32_t& deleted_idx = iter.get();
				if (deleted_idx == 0xFFFF'FFFF) {
					deleted_idx = index;
					return;
				}
			}

			uint32_t& new_deleted_index = deleted.emplace_back();
			new_deleted_index = index;
		}
	}

	/*void erase(T* pointer_to_elem_memory)
	{
		
	}*/

	void clear()
	{
		// mark as deleted
		for (auto iter = elems.begin(); iter != elems.end(); iter.next()) {
			DeferredVectorNode<T>& node = iter.get();
			node.is_deleted = true;
		}

		elems.clear();
		deleted.clear();

		_size = 0;
		_first_index = 0;
		_last_index = 0;
	}

	T& operator[](uint32_t idx)
	{
		return elems[idx].elem;
	}

	SparseVectorIterator<T> begin()
	{
		SparseVectorIterator<T> iter;
		iter._parent_vector = this;
		iter._current_idx = _first_index;

		return iter;
	}

	SparseVectorIterator<T> end()
	{
		SparseVectorIterator<T> iter;
		iter._parent_vector = this;
		iter._current_idx = _last_index + 1;

		return iter;
	}

	// ease of refactoring
	inline uint32_t size()
	{
		return _size;
	}

	inline uint32_t capacity()
	{
		return elems.capacity;
	}
};