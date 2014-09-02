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

