#include <std_include.hpp>
#include "loader/component_loader.hpp"
#include "motd.hpp"
#include "images.hpp"

#include <utils/hook.hpp>
#include <utils/http.hpp>
#include <utils/io.hpp>

#include <resource.hpp>

namespace motd
{
	namespace
	{
		std::string motd_resource = utils::nt::load_resource(DW_MOTD);
		std::future<std::optional<std::string>> motd_future;
		std::string marketing_featured_msg_str;
	}

	std::string get_text()
	{
		try
		{
			return motd_future.get().value_or(motd_resource);
		}
		catch (std::exception&)
		{
		}

		return motd_resource;
	}

	utils::hook::detour marketing_get_message_hook;

	bool marketing_get_message_stub(int controllerIndex, int locationID, char* messageText, int messageTextLength)
	{
		if(marketing_featured_msg_str.empty()) return false;

		strncpy(messageText, marketing_featured_msg_str.data(), messageTextLength);

		return true;
	}
	
	class component final : public component_interface
	{
	public:
		void post_load() override
		{
			motd_future = utils::http::get_data_async("https://xlabs.dev/s1/motd.txt");
			std::thread([]()
			{
				auto data = utils::http::get_data("https://xlabs.dev/s1/motd.png");
				if(data)
				{
					images::override_texture("iotd_image", data.value());
				}
				
				auto featured_optional = utils::http::get_data("https://xlabs.dev/s1/featured.json");
				if (featured_optional)
				{
					marketing_featured_msg_str = featured_optional.value();
				}
				
			}).detach();
		}
		
		void post_unpack() override
		{
			marketing_get_message_hook.create(0x140126930, marketing_get_message_stub); // not sure why but in s1x, client doesnt ask for maketing messages from demonware even with marketing_active set to true
		}
	};
}

REGISTER_COMPONENT(motd::component)
