defmodule ImageEx.Gif do
  alias ImageEx.Base

  def create(width, height, bit_depth) do
    Base.create_gif(width, height, bit_depth)
  end

  def create!(width, height, bit_depth) do
    case create(width, height, bit_depth) do
      {:ok, ref} -> ref
      {:error, err} -> raise err
      err -> raise "Unknown error: #{inspect(err)}"
    end
  end

  def add_frame(ref, data, delay) do
    Base.gif_add_frame(ref, data, delay)
  end

  def add_frame!(ref, data, delay) do
    case add_frame(ref, data, delay) do
      {:ok, ref} -> ref
      {:error, err} -> raise err
      err -> raise "Unknown error: #{inspect(err)}"
    end
  end

  def finish(ref) do
    Base.gif_end(ref)
  end

  def finish!(ref) do
    case finish(ref) do
      {:ok, ref} -> ref
      {:error, err} -> raise err
      err -> raise "Unknown error: #{inspect(err)}"
    end
  end
end
