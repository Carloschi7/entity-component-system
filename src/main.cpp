#include <iostream>
#include <map>
#include <vector>
#include <memory>
#include "red_black_tree.h"

//Simple yet pretty effective for our needs algorithm
constexpr static u64 string_hash(const char* str, u32 size)
{
	//Random prime number
	u64 hash = 11447;

	for(u32 i = 0; i < size; i++) {
		hash = (hash << 5) + hash + str[i];
	}

	return hash;
}

template<class Type>
static u64 type_hash()
{
	const char* name = typeid(Type).name();
	return string_hash(name, sizeof(name));
}

//Basic container used to store user defined types
struct GenericStorage 
{
	GenericStorage(u64 hash_of_type) : hash_of_type{ hash_of_type } {}
	const u64 hash_of_type;
};

template<typename Type>
class TypeStorage : public GenericStorage
{
	using map_type         =    std::map<u64, Type>;
	using iterator         =    typename map_type::iterator;
	using const_iterator   =    typename map_type::const_iterator;
public:
	TypeStorage() : GenericStorage(type_hash<Type>()) {}
	virtual ~TypeStorage() 
	{
	}

	template<typename... Args>
	decltype(auto) emplace(u64 entity, Args&&... args) {
		assert(local_storage.find(entity) == local_storage.end());
		return local_storage.emplace(entity, std::forward<Args>(args)...);
	}

	template<typename... Args>
	decltype(auto) emplace_or_replace(u64 entity, Args&&... args) {
		if (local_storage.find(entity) != local_storage.end())
			local_storage.erase(entity);

		return local_storage.emplace(entity, std::forward<Args>(args)...);
	}

	void destroy(u64 entity) {
		assert(local_storage.find() != local_storage.end());
		local_storage.erase(entity);
	}

	decltype(auto) find(u64 entity) {
		return local_storage.find(entity);
	}

	decltype(auto) find(u64 entity) const {
		return local_storage.find(entity);
	}

	iterator begin() {
		return local_storage.begin();
	}

	iterator end() {
		return local_storage.end();
	}

	const_iterator begin() const {
		return local_storage.begin();
	}

	const_iterator end() const {
		return local_storage.end();
	}

	const_iterator cbegin() const {
		return local_storage.begin();
	}

	const_iterator cend() const {
		return local_storage.end();
	}

	Type& operator[](u64 key) {
		return local_storage[key];
	}

	const Type& operator[](u64 key) const {
		return local_storage[key];
	}

private:
	map_type local_storage;
};

template<typename Type>
static TypeStorage<Type>& storage_cast(std::shared_ptr<GenericStorage> storage)
{
	assert(storage != nullptr);
	assert(storage->hash_of_type == type_hash<Type>());
	return static_cast<TypeStorage<Type>&>(*storage);
}

class Registry
{
public:
	Registry() : entity_generational_index{0} {}

	template <class Type, class... Args>
	Type& add_component(u64 entity, Args&&... args) noexcept 
	{
		static_assert(!std::is_same_v<Type, void>, "void allocation not possible");
		
		const u64 hash_of_type = type_hash<Type>();
		auto iter = type_register.find(hash_of_type);
		std::shared_ptr<GenericStorage> type_map;

		if(iter == type_register.end()){
			type_map = std::make_shared<TypeStorage<Type>>();
			storage_cast<Type>(type_map).emplace(entity, std::forward<Args>(args)...);
			type_register[hash_of_type] = type_map;
		} else {
			type_map = iter->second;
			storage_cast<Type>(type_map).emplace(entity, std::forward<Args>(args)...);
		}

		return storage_cast<Type>(type_map)[entity];
	}

	template<class Type>
	[[nodiscard]] Type& get_component(u64 entity)
	{
		auto type_iter = type_register.find(type_hash<Type>()); 
		assert(type_iter != type_register.end());

		TypeStorage<Type>& storage_of_type = storage_cast<Type>(type_iter->second);
		auto entity_iter = storage_of_type.find(entity);
		assert(entity_iter != storage_of_type.end());

		return storage_of_type[entity];
	}

	template<class Type>
	[[nodiscard]] const Type& get_component(u64 entity) const
	{
		auto type_iter = type_register.find(type_hash<Type>()); 
		assert(type_iter != type_register.end());

		TypeStorage<Type>& storage_of_type = storage_cast<Type>(type_iter->second);
		auto entity_iter = sub_map.find(entity);
		assert(entity_iter != sub_map.end());

		return storage_of_type[entity];
	}

	template<class Type, class... Args>
	[[nodiscard]] Type& replace_component(u64 entity, Args&&... args)
	{
		auto type_iter = type_register.find(type_hash<Type>());
		assert(type_iter != type_register.end());

		TypeStorage<Type>& storage_of_type = storage_cast<Type>(type_iter->second);
		auto attribute_to_delete = storage_of_type.find(entity);
		assert(attribute_to_delete != storage_of_type.end());

		storage_of_type.destroy(attribute_to_delete);
		
		storage_of_type.emplace(entity, std::forward<Args>(args)...);
		return storage_of_type[entity];
	}

	template<class Type>
	void delete_component(u64 entity)
	{
		auto type_iter = type_register.find(type_hash<Type>());
		assert(type_iter != type_register.end());

		TypeStorage<Type>& storage_of_type = storage_cast<Type>(type_iter->second);
		auto attribute_to_delete = storage_of_type.find(entity);
		assert(attribute_to_delete != storage_of_type.end());

		storage_of_type.destroy(attribute_to_delete);
	}

	u64 create_entity() const
	{
		return entity_generational_index++;
	}
private:
	mutable u64 entity_generational_index;
	std::map<u64, std::shared_ptr<GenericStorage>> type_register;
};


struct Vector2D
{
	f32 x, y;
};

int main()
{
	//Registry reg;
	//u64 entity = reg.create_entity(); 

	//reg.add_component<Vector2D>(entity, Vector2D{});
	//Vector2D& vec = reg.get_component<Vector2D>(entity);
	//int k = 0;

	RedBlackTree<int> tree;
	tree.emplace_node(0, int{});
	tree.emplace_node(1, int{});
	tree.emplace_node(2, int{});
	tree.emplace_node(3, int{});
	tree.emplace_node(4, int{});
	tree.emplace_node(5, int{});
	tree.emplace_node(6, int{});
	tree.emplace_node(7, int{});

	tree.delete_node(7);
	tree.emplace_node(7, int{});

	//tree.delete_node(5);
	//tree.delete_node(6);
	//tree.delete_node(7);
	//tree.delete_node(1);
	//tree.delete_node(4);
	//tree.delete_node(2);
	//tree.delete_node(3);
	//tree.delete_node(0);

	return 0;
}


#undef assert




