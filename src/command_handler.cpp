#include "command_handler.hpp"

#include <yarrr/command.hpp>
#include <yarrr/log.hpp>

namespace yarrrs
{

void
CommandHandler::execute( const yarrr::Command& command, Player& player ) const
{
  thelog( yarrr::log::debug )( "Looking for command executor for", command.command() );

  m_handlers.at( command.command() )( command, player );
}

void
CommandHandler::register_handler( const std::string& command, Handler handler )
{
  thelog( yarrr::log::debug )( "Registering command executor for", command );
  m_handlers[ command ] = handler;
}

}

