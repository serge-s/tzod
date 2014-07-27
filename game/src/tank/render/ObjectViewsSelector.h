#pragma once
#include <memory>
#include <vector>

class GC_Actor;
struct ObjectZFunc;
struct ObjectRFunc;

struct ObjectView
{
	std::unique_ptr<ObjectZFunc> zfunc;
	std::unique_ptr<ObjectRFunc> rfunc;
	ObjectView(ObjectView &&) = default;
	ObjectView(std::unique_ptr<ObjectZFunc> zf, std::unique_ptr<ObjectRFunc> rf);
	~ObjectView();
};

class ObjectViewsSelector
{
public:
	typedef std::vector<ObjectView> ViewCollection;
	template <class T>
	void AddView(std::unique_ptr<ObjectZFunc> zfunc, std::unique_ptr<ObjectRFunc> rfunc)
	{
		auto type = T::GetTypeStatic();
		if (_type2views.size() <= type)
			_type2views.resize(type + 1);
		_type2views[type].emplace_back(std::move(zfunc), std::move(rfunc));
	}
	const ViewCollection* GetViews(const GC_Actor &actor) const;
private:
	std::vector<ViewCollection> _type2views;
};