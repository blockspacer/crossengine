#pragma once

#include <vector>
#include "flatbuffers/flatbuffers.h"

BEGIN_XE_NAMESPACE

template <typename TClient, typename THandler>
class Backplane
{
public:
	Backplane(TClient &client) { }
	Backplane(Backplane const &) = delete;
	Backplane(Backplane &&) = delete;
	void post(flatbuffers::DetachedBuffer buffer);
	void receive(THandler &handler);

private:
	std::vector<flatbuffers::DetachedBuffer> _pending;
	TClient _client;
};

template <typename TClient, typename THandler>
void Backplane<TClient, THandler>::post(flatbuffers::DetachedBuffer buffer)
{
	_client.send(buffer);
	_pending.push_back(std::move(buffer));
}

template <typename TClient, typename THandler>
void Backplane<TClient, THandler>::receive(THandler &handler)
{
	for (auto &buffer : _pending)
	{
		handler.receiveBuffer(buffer);
	}
	_pending.clear();
}

END_XE_NAMESPACE
