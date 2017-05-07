#include "global/NumberGenerator.h"
#include "model/ModelSettings.h"
#include "model/context/SimulationParameters.h"
#include "model/context/UnitContext.h"

#include "TokenImpl.h"

TokenImpl::TokenImpl(UnitContext * context)
	: _context(context)
{
	_memory = QByteArray(context->getSimulationParameters()->tokenMemorySize, 0);
}

TokenImpl::TokenImpl(UnitContext* context, qreal energy, QByteArray const& memory)
	: _energy(energy), _context(context)
{
	_memory = memory.left(context->getSimulationParameters()->tokenMemorySize);
}

void TokenImpl::init(UnitContext * context)
{
	_context = context;
}

TokenImpl* TokenImpl::duplicate() const
{
	TokenImpl* newToken(new TokenImpl(_context));
	for (int i = 0; i < _context->getSimulationParameters()->tokenMemorySize; ++i)
		newToken->_memory[i] = _memory[i];
	newToken->_energy = _energy;

	return newToken;
}

int TokenImpl::getTokenAccessNumber() const
{
	return _memory[0] % _context->getSimulationParameters()->cellMaxTokenBranchNumber;
}

void TokenImpl::setTokenAccessNumber(int i)
{
	_memory[0] = i;
}

void TokenImpl::setEnergy(qreal energy)
{
	_energy = energy;
}

qreal TokenImpl::getEnergy() const
{
	return _energy;
}

QByteArray & TokenImpl::getMemoryRef() 
{
	return _memory;
}

void TokenImpl::serializePrimitives(QDataStream& stream) const
{
	stream << _memory << _energy;
}

void TokenImpl::deserializePrimitives(QDataStream& stream)
{
	stream >> _memory >> _energy;
	auto memSize = _context->getSimulationParameters()->tokenMemorySize;
	_memory = _memory.left(memSize);
	_memory.resize(memSize);
}
