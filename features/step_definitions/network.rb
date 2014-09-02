
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

