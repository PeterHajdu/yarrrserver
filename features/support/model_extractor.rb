require 'zmq'

class ModelExtractor

  def initialize( endpoint )
    @endpoint = endpoint
    @context = ZMQ::Context.new
    @socket = @context.socket( :REQ )
    @socket.connect @endpoint
    @socket.rcvtimeo = 3000
  end

  def extract_model
    @socket.send ""
    response = @socket.recv
    if response == nil then
      raise "Unable to extract model."
    end

    return response
  end

end

