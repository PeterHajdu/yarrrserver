
def sleep_a_bit
  sleep 1.0 / 4.0
end

Given(/^a tcp connection$/) do
  puts "connecting to localhost:#{@server_port}"
  @client_connection = TCPSocket.new 'localhost', @server_port
  sleep_a_bit
end

When(/^I close the connection$/) do
  @client_connection.close
  sleep_a_bit
end

When(/^I send "(.*?)" on the connection$/) do | message |
  @client_connection.send message, 0
  sleep_a_bit
end

Then(/^the connection should be closed$/) do
  expect( @client_connection.gets ).to be nil
end

When(/^I send the "(.*?)" thenet message on the connection$/) do | message |
  length = [ message.length ]
  thenet_message = length.pack( "N" ) + message
  @client_connection.send thenet_message, 0
end

Then(/^I should be able to connect to port (\d+)$/) do | port |
  expect( TCPSocket.new 'localhost', port ).not_to be nil
end

