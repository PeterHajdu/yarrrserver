
When(/^I start the server with command line parameter (.*)$/) do | parameter |
  @yarrr_server = ProcessRunner.new(
    {"LUA_PATH" => "/home/tacsko/.luarocks/share/lua/5.2/?.lua"},
    "yarrrserver #{parameter}" )
  @yarrr_server.start
end

When(/^I start the server without any command line parameter$/) do
  step "I start the server with command line parameter "
end

Given(/^a running server$/) do
  @server_port = 21346
  step "I start the server with command line parameter --port #{@server_port} --notify #{@notification_file_path}"
end

Then(/^I should see (.+)$/) do | pattern |
  expect( @yarrr_server.output ).to match( /#{pattern}/ )
end

Then(/^the server should be running$/) do
  expect( @yarrr_server.is_running ).to be true
end

Then(/^the server should not be running$/) do
  expect( @yarrr_server.is_running ).to be false
end

Then(/^It should print the usage text to the terminal$/) do
  step "I should see yarrrserver --port"
end

When(/^I start a client$/) do
  @yarrr_client = ProcessRunner.new(
    {},
    "yarrrclient --text --server localhost:#{@server_port}" )
  @yarrr_client.start
  sleep 1.0
end

Then(/^I should receive a notification containing "(.*?)"$/) do | notification_message |
  @notification_message = @notification_file.gets
  expect( @notification_message ).to match( /#{notification_message}/ )
end

