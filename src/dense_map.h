#pragma once
#include "red_black_tree.h"

template<typename Type, typename Index, bool is_reverse>
class MapIterator
{
	using NodeType = TreeNode<Type, Index>;
	using TreeType = RedBlackTree<Type, Index>;
public:
	MapIterator(NodeType* start) : node{start} {}
	MapIterator(const MapIterator&) = default;
	
	MapIterator& operator++() 
	{
		node = is_reverse ? node->find_prev() : node->find_next();
		return *this;
	}

	MapIterator& operator++(int)
	{
		node = is_reverse ? node->find_prev() : node->find_next();
		return *this;
	}

	MapIterator& operator--()
	{
		node = is_reverse ? node->find_next() : node->find_prev();
		return *this;
	}

	MapIterator& operator--(int)
	{
		node = is_reverse ? node->find_next() : node->find_prev();
		return *this;
	}

	bool operator==(const MapIterator& other)
	{
		return node == other.node;
	}

	bool operator!=(const MapIterator& other)
	{
		return node != other.node;
	}

	NodeType* _Get_Node() const { return node; }

private:
	NodeType* node;
};

template <typename Key, typename Type>
class DenseMap
{
	static_assert(std::is_integral_v<Key>, "for dense maps, the Key type needs to be an integral type, dense_map is not responsible for hash calculations");

	using Tree =                RedBlackTree<Type>;
	using Node =                TreeNode<Type, Key>;
	using Iterator =            MapIterator<Type, Key, false>;
	using ReverseIterator =     MapIterator<Type, Key, true>;
public:
	DenseMap() = default;
	
	template<class... Args>
	Type& emplace(Key index, Args&&... args)
	{
		//Replace if exists
		static_assert(std::is_constructible_v<Type, Args...>, "Type needs to be constructible with the given arguments");
		delete_if_exists(index);
		return map_tree.emplace_node(index, std::forward<Args>(args)...);
	}

	Type& get_at(Key index)
	{
		Node* node = map_tree.find_node(index);
		assert(node != nullptr && node->content != nullptr);
		return *node->content;
	}

	void erase(Key index) 
	{
		map_tree.delete_node(index);
	}

	void erase(Iterator it)
	{
		map_tree.delete_node(it._Get_Node());
	}

	void erase(ReverseIterator it)
	{
		map_tree.delete_node(it._Get_Node());
	}

	void erase(Iterator first, const Iterator last)
	{
		check_range_valid(first, last);
		for (; first != last; ++first) {
			erase(first);
		}
	}

	void erase(ReverseIterator first, const ReverseIterator last)
	{
		check_range_valid(first, last);
		for (; first != last; ++first) {
			erase(first);
		}
	}

	Iterator begin()
	{
		return Iterator{ map_tree.find_first() };
	}

	Iterator end()
	{
		return Iterator{ nullptr };
	}

	Iterator cbegin() const
	{
		return Iterator{ map_tree.find_first() };
	}

	Iterator cend() const
	{
		return Iterator{ nullptr };
	}

	ReverseIterator rbegin()
	{
		return ReverseIterator{ map_tree.find_last() };
	}

	ReverseIterator rend()
	{
		return ReverseIterator{ nullptr };
	}

	ReverseIterator crbegin() const
	{
		return ReverseIterator{ map_tree.find_last() };
	}

	ReverseIterator crend() const
	{
		return ReverseIterator{ nullptr };
	}

	Type& operator[](Key index)
	{
		//Only available for default constructed objects
		static_assert(std::is_default_constructible_v<Type>, "building objects without default constructors via the [] operator is not possible");
		Node* node = map_tree.find_node(index);
		if(node == nullptr){
			return map_tree.emplace_node(index);
		}

		return *node->content;
	}

	const Type& operator[](const Key index) const
	{
		Node* node = map_tree.find_node(index);
		assert(node != nullptr && node->content != nullptr);
		return *node->content;
	}


private:
	void check_range_valid(const Iterator first, const Iterator second)
	{
		return first._Get_Node()->index < second._Get_Node()->index;
	}

	void check_range_valid(const ReverseIterator first, const ReverseIterator second)
	{
		return first._Get_Node()->index > second._Get_Node()->index;
	}

	void delete_if_exists(Key index)
	{
		if (map_tree.find_node(index))
			map_tree.delete_node(index);
	}
private:
	Tree map_tree;
};
