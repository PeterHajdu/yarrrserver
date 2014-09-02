
When(/^I start the server with command line parameter (.*)$/) do | parameter |
  @yarrr_server = ProcessRunner.new(
    {},
    "yarrrserver #{parameter}" )
  @yarrr_server.start
end

When(/^I start the server$/) do
  step "I start the server with command line parameter "
end

Given(/^a running server$/) do
  step "I start the server with command line parameter "
end

Then(/^I should see (.+)$/) do | pattern |
  expect( @yarrr_server.output ).to match( /#{pattern}/ )
end

Then(/^I should not see (.+)$/) do | pattern |
  expect(  @yarrr_server.output ).not_to match( /#{pattern}/ )
end

Then(/^the server should be running$/) do
  expect( @yarrr_server.is_running ).to be true
end

When(/^I wait one second$/) do
  sleep 1
end

