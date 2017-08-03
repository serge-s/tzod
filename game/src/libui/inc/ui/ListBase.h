#pragma once

#include <string>
#include <vector>

namespace UI
{

struct ListDataSourceListener
{
	virtual void OnDeleteAllItems() = 0;
	virtual void OnDeleteItem(int idx) = 0;
	virtual void OnAddItem() = 0;
};

struct ListDataSource
{
	virtual void AddListener(ListDataSourceListener *cb) = 0;
	virtual void RemoveListener(ListDataSourceListener *cb) = 0;
	virtual int GetItemCount() const = 0;
	virtual int GetSubItemCount(int index) const = 0;
	virtual size_t GetItemData(int index) const = 0;
	virtual std::string_view GetItemText(int index, int sub) const = 0;
	virtual int FindItem(std::string_view text) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

class ListDataSourceDefault : public ListDataSource
{
public:
	// ListDataSource interface
	virtual void AddListener(ListDataSourceListener *cb);
	virtual void RemoveListener(ListDataSourceListener *cb);
	virtual int GetItemCount() const;
	virtual int GetSubItemCount(int index) const;
	virtual size_t GetItemData(int index) const;
	virtual std::string_view GetItemText(int index, int sub) const;
	virtual int FindItem(std::string_view text) const;

	// extra
	int  AddItem(std::string_view str, size_t data = 0);
	void SetItemText(int index, int sub, std::string_view str);
	void SetItemData(int index, size_t data);
	void DeleteItem(int index);
	void DeleteAllItems();
	void Sort();

private:
	struct Item
	{
		std::vector<std::string> text;
		size_t data;
	};
	std::vector<Item> _items;
	ListDataSourceListener *_listener = nullptr;
};

} // end of namespace UI
