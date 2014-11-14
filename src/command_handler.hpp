#pragma once

#include <string>
#include <functional>
#include <unordered_map>

namespace yarrr
{

class Command;

}

namespace yarrrs
{

class Player;


class CommandHandler
{
  public:
    typedef std::tuple< bool, std::string > Result;
    Result execute( const yarrr::Command&, Player& ) const;

    typedef std::function< Result( const yarrr::Command&, Player& ) > Handler;
    void register_handler( const std::string& command, Handler handler );

  private:
    std::unordered_map< std::string, Handler > m_handlers;

};

}

