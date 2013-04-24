#ifndef __STAT_COLLECTOR_HH__
#define __STAT_COLLECTOR_HH__

#include <map>
#include <string>

class StatCollector
{
public:
    void set_id(uint32_t id)
    { m_id = id; }

    void incr(const std::string & key)
    {
        MapType::const_iterator it = m_stats.find(key);
        if(it == m_stats.end())
            m_stats[key] = 0;

        ++m_stats[key];
    }

    void max(const std::string & key, unsigned int val)
    {
        MapType::const_iterator it = m_stats.find(key);
        if(it == m_stats.end())
            m_stats[key] = val;
        else
            m_stats[key] = std::max(it->second, val);
    }

    unsigned int get(const std::string & key) const
    {
        MapType::const_iterator it = m_stats.find(key);
        if(it == m_stats.end())
            return 0;
        return it->second;
    }

    std::vector<std::string> keys() const
    {
        std::vector<std::string> out;
        for(MapType::const_iterator it = m_stats.begin();
            it != m_stats.end();
            ++it)
        {
            out.push_back(it->first);
        }
        return out;
    }

    void dump(std::ostream & out, const std::string & prefix = "") const
    {
        out << "  node " << m_id << ":\n";
        for(MapType::const_iterator it = m_stats.begin();
            it != m_stats.end();
            ++it)
        {
            out << prefix << "   " << it->first << " = " << it->second << "\n";
        }
    }

private:
    uint32_t m_id;

    typedef std::map<std::string, unsigned int> MapType;
    MapType m_stats;
};

#endif

