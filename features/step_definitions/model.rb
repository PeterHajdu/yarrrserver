
Then(/^I should be able to get model information from remote endpoint (.+)$/) do | endpoint |
  @model_extractor = ModelExtractor.new( endpoint )
  @model = @model_extractor.extract_model
end

