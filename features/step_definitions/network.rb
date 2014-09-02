
Given(/^a tcp connection$/) do
  @client_connection = TCPSocket.new 'localhost', 2001
end

When(/^I close the connection$/) do
  @client_connection.close
end

