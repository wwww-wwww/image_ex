defmodule ImageEx.Base do
  @compile {:autoload, false}
  @on_load {:init, 0}

  @priv_paths [
    :code.priv_dir(:image_ex),
    '_build/prod/lib/image_ex/priv/',
    '_build/dev/lib/image_ex/priv/'
  ]

  @make_default 'libimage_ex_nif'
  @msvc_debug 'Debug/image_ex_nif'
  @msvc_release 'Release/image_ex_nif'

  def init do
    case load_nif() do
      :ok ->
        :ok

      err ->
        raise """
        #{inspect(err)}
        An error occurred when loading image_ex.
        Make sure you have a C compiler and Erlang 20 installed.
        """
    end
  end

  defp load_nif() do
    [@make_default, @msvc_release, @msvc_debug]
    |> Enum.map(fn x -> Enum.map(@priv_paths, fn y -> :filename.join(y, x) end) end)
    |> Enum.reduce([], fn x, acc -> acc ++ x end)
    |> Enum.reduce_while([], fn x, acc ->
      case :erlang.load_nif(x, 0) do
        :ok -> {:halt, :ok}
        err -> {:cont, acc ++ [err]}
      end
    end)
  end

  def create_gif(_width, _height, _bit_depth), do: fail()
  def gif_add_frame(_state, _data, _delay), do: fail()
  def gif_end(_state), do: fail()

  def png_reencode(_data), do: fail()

  defp fail, do: raise("native function error")
end
