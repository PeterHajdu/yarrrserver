
def sleep_a_bit
  sleep 1.0 / 4.0
end

Given(/^a tcp connection$/) do
  @client_connection = TCPSocket.new 'localhost', 2001
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
  p thenet_message
  @client_connection.send thenet_message, 0
end

