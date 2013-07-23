// Copyright (c) 2012, Robert Escriva
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Replicant nor the names of its contributors may be
//       used to endorse or promote products derived from this software without
//       specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// STL
#include <algorithm>

// Replicant
#include "daemon/configuration_manager.h"

using replicant::configuration;
using replicant::configuration_manager;

configuration_manager :: configuration_manager()
    : m_configs()
    , m_proposals()
{
}

configuration_manager :: configuration_manager(const configuration_manager& other)
    : m_configs(other.m_configs)
    , m_proposals(other.m_proposals)
{
}

configuration_manager :: ~configuration_manager() throw ()
{
}

const configuration&
configuration_manager :: stable() const
{
    assert(!m_configs.empty());
    return m_configs.front();
}

const configuration&
configuration_manager :: latest() const
{
    assert(!m_configs.empty());
    return m_configs.back();
}

void
configuration_manager :: get_all_nodes(std::vector<chain_node>* nodes) const
{
    for (std::list<configuration>::const_iterator c = m_configs.begin();
            c != m_configs.end(); ++c)
    {
        for (const chain_node* n = c->members_begin(); n != c->members_end(); ++n)
        {
            nodes->push_back(*n);
        }
    }

    std::sort(nodes->begin(), nodes->end());
    std::vector<chain_node>::iterator it = std::unique(nodes->begin(), nodes->end());
    nodes->resize(it - nodes->begin());
}

void
configuration_manager :: get_config_chain(std::vector<configuration>* config_chain) const
{
    config_chain->clear();

    for (std::list<configuration>::const_iterator c = m_configs.begin();
            c != m_configs.end(); ++c)
    {
        config_chain->push_back(*c);
    }
}

bool
configuration_manager :: get_proposal(uint64_t proposal_id,
                                      uint64_t proposal_time,
                                      configuration* config) const
{
    std::list<proposal>::const_iterator prop = m_proposals.begin();

    while (prop != m_proposals.end() &&
           prop->id != proposal_id &&
           prop->time != proposal_time)
    {
        ++prop;
    }

    if (prop == m_proposals.end())
    {
        return false;
    }

    std::list<configuration>::const_iterator conf = m_configs.begin();

    while (conf != m_configs.end() &&
           conf->version() != prop->version)
    {
        ++conf;
    }

    if (conf == m_configs.end())
    {
        return false;
    }

    *config = *conf;
    return true;
}

void
configuration_manager :: get_proposals(std::vector<proposal>* proposals) const
{
    proposals->clear();

    for (std::list<proposal>::const_iterator p = m_proposals.begin();
            p != m_proposals.end(); ++p)
    {
        proposals->push_back(*p);
    }
}

bool
configuration_manager :: is_compatible(const configuration* configs,
                                       size_t configs_sz) const
{
    std::list<configuration>::const_iterator iter1 = m_configs.begin();
    size_t iter2 = 0;

    while (iter1 != m_configs.end() && iter2 < configs_sz)
    {
        if (*iter1 != configs[iter2])
        {
            return false;
        }

        ++iter1;
        ++iter2;
    }

    return true;
}

bool
configuration_manager :: contains_quorum_of_all(const configuration& config) const
{
    for (std::list<configuration>::const_iterator it = m_configs.begin();
            it != m_configs.end(); ++it)
    {
        if (!config.quorum_of(*it))
        {
            return false;
        }
    }

    return true;
}

bool
configuration_manager :: any(bool (configuration::*func)(uint64_t) const, uint64_t token) const
{
    for (std::list<configuration>::const_iterator it = m_configs.begin();
            it != m_configs.end(); ++it)
    {
        if (((*it).*func)(token))
        {
            return true;
        }
    }

    return false;
}

void
configuration_manager :: advance(const configuration& config)
{
    while (!m_configs.empty() && m_configs.front().version() < config.version())
    {
        m_configs.pop_front();
    }

    assert(!m_configs.empty());
    assert(m_configs.front() == config);
    std::list<proposal>::iterator prop = m_proposals.begin();

    while (prop != m_proposals.end())
    {
        if (prop->version <= config.version())
        {
            prop = m_proposals.erase(prop);
        }
        else
        {
            ++prop;
        }
    }
}

void
configuration_manager :: merge(uint64_t proposal_id,
                               uint64_t proposal_time,
                               const configuration* configs,
                               size_t configs_sz)
{
    assert(configs_sz > 0);
    std::list<configuration>::iterator iter = m_configs.begin();

    for (size_t i = 0; i < configs_sz; ++i)
    {
        if (iter == m_configs.end())
        {
            m_configs.push_back(configs[i]);
        }
        else
        {
            assert(*iter == configs[i]);
            ++iter;
        }
    }

    m_proposals.push_back(proposal(proposal_id, proposal_time, configs[configs_sz - 1].version()));
}

void
configuration_manager :: reject(uint64_t proposal_id, uint64_t proposal_time)
{
    uint64_t max_version = m_configs.front().version();
    std::list<proposal>::iterator prop = m_proposals.begin();
    std::list<proposal>::iterator to_erase = m_proposals.end();

    while (prop != m_proposals.end())
    {
        if (prop->id == proposal_id && prop->time == proposal_time)
        {
            to_erase = prop;
        }
        else
        {
            max_version = std::max(max_version, prop->version);
        }

        ++prop;
    }

    assert(to_erase != m_proposals.end());
    m_proposals.erase(to_erase);

    while (!m_configs.empty() && m_configs.back().version() > max_version)
    {
        m_configs.pop_back();
    }

    assert(!m_configs.empty());
}

void
configuration_manager :: reset(const configuration& config)
{
    m_configs.clear();
    m_proposals.clear();
    m_configs.push_back(config);
}

configuration_manager&
configuration_manager :: operator = (const configuration_manager& rhs)
{
    m_configs = rhs.m_configs;
    m_proposals = rhs.m_proposals;
    return *this;
}

bool
replicant :: operator < (const configuration_manager::proposal& lhs,
                         const configuration_manager::proposal& rhs)
{
    return lhs.version < rhs.version;
}

std::ostream&
replicant :: operator << (std::ostream& lhs, const configuration_manager& rhs)
{
    for (std::list<configuration>::const_iterator conf = rhs.m_configs.begin();
            conf != rhs.m_configs.end(); ++conf)
    {
        lhs << "    " << *conf << std::endl;
    }

    for (std::list<configuration_manager::proposal>::const_iterator prop = rhs.m_proposals.begin();
            prop != rhs.m_proposals.end(); ++prop)
    {
        lhs << "    id=" << prop->id << " time=" << prop->time << " version=" << prop->version << std::endl;
    }

    return lhs;
}

e::buffer::packer
replicant :: operator << (e::buffer::packer lhs, const configuration_manager& rhs)
{
    uint32_t configs_sz = rhs.m_configs.size();
    lhs = lhs << configs_sz;

    for (std::list<configuration>::const_iterator conf = rhs.m_configs.begin();
            conf != rhs.m_configs.end(); ++conf)
    {
        lhs = lhs << *conf;
    }

    uint32_t proposals_sz = rhs.m_proposals.size();
    lhs = lhs << proposals_sz;

    for (std::list<configuration_manager::proposal>::const_iterator prop = rhs.m_proposals.begin();
            prop != rhs.m_proposals.end(); ++prop)
    {
        lhs = lhs << prop->id << prop->time << prop->version;
    }

    return lhs;
}

e::unpacker
replicant :: operator >> (e::unpacker lhs, configuration_manager& rhs)
{
    rhs.m_configs.clear();
    rhs.m_proposals.clear();

    uint32_t configs_sz;
    lhs = lhs >> configs_sz;

    if (lhs.error())
    {
        return lhs;
    }

    for (size_t i = 0; i < configs_sz; ++i)
    {
        configuration c;
        lhs = lhs >> c;

        if (lhs.error())
        {
            return lhs;
        }

        rhs.m_configs.push_back(c);
    }

    uint32_t proposals_sz;
    lhs = lhs >> proposals_sz;

    for (size_t i = 0; i < proposals_sz; ++i)
    {
        configuration_manager::proposal p;
        lhs = lhs >> p.id >> p.time >> p.version;

        if (lhs.error())
        {
            return lhs;
        }

        rhs.m_proposals.push_back(p);
    }

    return lhs;
}

size_t
replicant :: pack_size(const configuration_manager& cm)
{
    size_t sz = 2 * sizeof(uint32_t)
              + cm.m_proposals.size() * 3 * sizeof(uint64_t);

    for (std::list<configuration>::const_iterator conf = cm.m_configs.begin();
            conf != cm.m_configs.end(); ++conf)
    {
        sz += pack_size(*conf);
    }

    return sz;
}
