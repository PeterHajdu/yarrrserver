class ModelExtractor

  def initialize( endpoint )
    @endpoint = endpoint
  end

  def extract_model
    response = `the-model-remote #{@endpoint}`
    if not $?.success? then
      raise "Unable to extract model."
    end

    return response
  end

end

