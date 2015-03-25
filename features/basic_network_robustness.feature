@smoke
Feature: Malicious traffic robustness
  To be able to sleep well
  As a yarrr developer
  I should be able to let malicious traffic hit the server

  Scenario: tcp connect and close
    Given a running server
    And a tcp connection
    When I close the connection
    Then the server should be running

  Scenario: invalid thenet message
    Given a running server
    And a tcp connection
    When I send "apple" on the connection
    Then the server should be running
    And the connection should be closed

  Scenario: invalid yarrr message in valid thenet message
    Given a running server
    And a tcp connection
    When I send the "apple" thenet message on the connection
    Then the connection should be closed
    And the server should be running

  Scenario: user logs in and out
    Given a running server
    And a running client
    When I stop the client
    Then the server should be running

  Scenario: more user logs in
    Given a running server
    When I start 2 clients
    Then the server should be running

