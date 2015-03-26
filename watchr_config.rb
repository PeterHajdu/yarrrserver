require 'libnotify'

def failure( command )
  Libnotify.show( :body => command, :urgency => :critical, :summary => "failure", :timeout => 2.5 )
  false
end

def success( command )
  Libnotify.show( :body => command, :urgency => :low, :summary => "success", :timeout => 2.5 )
  true
end

def run( command )
  if ( system( command ) )
    success( command )
  else
    failure( command )
  end
end

def deploy
  run( "compile" ) && run( "unittest" ) && run( "local_deploy" ) && run( "cucumber21" )
end

to_watch = [ '.*pp', 'CMakeLists.txt', '.*rb', '.*feature' ]

to_watch.each do | pattern |
  watch( pattern ) do | filename |
    deploy
  end
end

