#include <experimental/executor>
#include <experimental/thread_pool>
#include <iostream>
#include <string>

using std::experimental::dispatch;
using std::experimental::get_associated_executor;
using std::experimental::make_work;
using std::experimental::post;
using std::experimental::thread_pool;
using std::experimental::wrap;

// A function to asynchronously read a single line from an input stream.
template <class Handler>
void async_getline(std::istream& is, Handler handler)
{
  // Create executor_work for the handler's associated executor.
  auto work = make_work(handler);

  // Post a function object to do the work asynchronously.
  post([&is, work, handler=std::move(handler)]
      {
        std::string line;
        std::getline(is, line);

        // Pass the result to the handler, via the associated executor.
        dispatch(work.get_executor(),
            [line=std::move(line), handler=std::move(handler)]() mutable
            {
              handler(std::move(line));
            });
      });
}

// A function to asynchronously read multiple lines from an input stream.
template <class Handler>
void async_getlines(std::istream& is, std::string init, Handler handler)
{
  // Get the final handler's associated executor.
  auto ex = get_associated_executor(handler);

  // Use the associated executor for each operation in the composition.
  async_getline(is,
      wrap(ex,
        [&is, lines=std::move(init), handler=std::move(handler)]
        (std::string line) mutable
        {
          if (line.empty())
            handler(lines);
          else
            async_getlines(is, lines + line + "\n", std::move(handler));
        }));
}

int main()
{
  thread_pool pool;

  std::cout << "Enter text, terminating with a blank line:\n";

  async_getlines(std::cin, "",
      wrap(pool, [](std::string lines)
        {
          std::cout << "Lines:\n" << lines << "\n";
        }));

  pool.join();
}