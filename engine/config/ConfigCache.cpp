#include "inc/config/ConfigCache.h"

ConfVarNumber& InitIntType(ConfVarNumber &var, const char *type)
{
	//	if( type )
	//	{
	//		ConfVarNumber::NumberMeta meta;
	//		meta.type = type;
	//		var->SetMeta(&meta);
	//	}
	return var;
}

ConfVarNumber& InitIntRange(ConfVarNumber &var, int iMin, int iMax)
{
	//	ConfVarNumber::NumberMeta meta;
	//	meta.type = "integer";
	//	meta.fMin = (float) iMin;
	//	meta.fMax = (float) iMax;
	//	var->SetMeta(&meta);
	return var;
}

ConfVarNumber& InitFloat(ConfVarNumber &var)
{
	return var;
}

ConfVarBool& InitBool(ConfVarBool &var)
{
	return var;
}

ConfVarArray& InitArray(ConfVarArray &var)
{
	return var;
}

ConfVarTable& InitTable(ConfVarTable &var)
{
	return var;
}

ConfVarNumber& InitFloatRange(ConfVarNumber &var, float fMin, float fMax)
{
	//	ConfVarNumber::NumberMeta meta;
	//	meta.type = "float";
	//	meta.fMin = fMin;
	//	meta.fMax = fMax;
	//	var->SetMeta(&meta);
	return var;
}

ConfVarString& InitStrType(ConfVarString &var, const char *type)
{
	//	if( type )
	//	{
	//		ConfVarString::StringMeta meta;
	//		meta.type = type;
	//		var->SetMeta(&meta);
	//	}
	return var;
}
