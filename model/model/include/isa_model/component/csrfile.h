/*
 * Copyright lizhirui
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-10-14     lizhirui     the first version
 */

#pragma once
#include <utility>

#include "common.h"
#include "config.h"
#include "csr_base.h"
#include "csr_all.h"

namespace isa_model::component
{
	class csrfile : public if_print_t, public if_reset_t
	{
		private:
			typedef struct csr_item_t
			{
				bool readonly = false;
				std::shared_ptr<csr_base> csr;
			}csr_item_t;

			std::unordered_map<uint32_t, csr_item_t> csr_map_table;

			static bool csr_out_list_cmp(const std::pair<std::string, std::string> &a, const std::pair<std::string, std::string> &b)
			{
				if(std::isdigit(a.first[a.first.length() - 1]) && std::isdigit(b.first[b.first.length() - 1]))
				{
					size_t a_index = 0;
					size_t b_index = 0;

					for(auto i = a.first.length() - 1;i >= 0;i--)
					{
						if(!std::isdigit(a.first[i]))
						{
							break;
						}
						else
						{
							a_index = i;
						}

						if(i == 0)
						{
							break;
						}
					}

					for(auto i = b.first.length() - 1;i >= 0;i--)
					{
						if(!std::isdigit(b.first[i]))
						{
							break;
						}
						else
						{
							b_index = i;
						}

						if(i == 0)
						{
							break;
						}
					}

					auto a_left = a.first.substr(0, a_index);
					auto a_right = a.first.substr(a_index);
					auto b_left = b.first.substr(0, b_index);
					auto b_right = b.first.substr(b_index);
					auto a_int = 0;
					auto b_int = 0;
					std::istringstream a_stream(a_right);
					std::istringstream b_stream(b_right);
					a_stream >> a_int;
					b_stream >> b_int;
					return (a_left < b_left) || ((a_left == b_left) && (a_int < b_int));
				}
				else
				{
					return a.first < b.first;
				}
			}

		public:
			csrfile()
			{
			
			}

			virtual void reset()
			{
				for(auto iter = csr_map_table.begin();iter != csr_map_table.end();iter++)
				{
					iter->second.csr->reset();
				}
			}

			void map(uint32_t addr, bool readonly, std::shared_ptr<csr_base> csr)
			{
				verify(csr_map_table.find(addr) == csr_map_table.end());
				csr_item_t t_item;
				t_item.readonly = readonly;
				t_item.csr = csr;
				csr_map_table[addr] = t_item;
			}

			void write_sys(uint32_t addr, uint32_t value)
			{
				verify(!(csr_map_table.find(addr) == csr_map_table.end()));
				csr_map_table[addr].csr->write(value);
			}

			uint32_t read_sys(uint32_t addr)
			{
				verify(!(csr_map_table.find(addr) == csr_map_table.end()));
				return csr_map_table[addr].csr->read();
			}

			bool write_check(uint32_t addr, uint32_t value)
			{
				if(csr_map_table.find(addr) == csr_map_table.end())
				{
					return false;
				}

				if(csr_map_table[addr].readonly)
				{
					return false;
				}

				return true;
			}

			bool write(uint32_t addr, uint32_t value)
			{
				if(!write_check(addr, value))
				{
					return false;
				}

				csr_map_table[addr].csr->write(value);
				return true;
            }

			bool read(uint32_t addr, uint32_t *value)
			{
				if(csr_map_table.find(addr) == csr_map_table.end())
				{
					return false;
				}

				*value = csr_map_table[addr].csr->read();
				return true;
			}

			virtual void print(std::string indent)
			{
				std::cout << indent << "CSR List:" << std::endl;
				std::vector<std::pair<std::string, std::string>> out_list;

				for(auto iter = csr_map_table.begin();iter != csr_map_table.end();iter++)
				{
					std::ostringstream stream;
					stream << indent << std::setw(15) << iter->second.csr->get_name() << "\t[0x" << fillzero(3) << outhex(iter->first) << ", " << (iter->second.readonly ? "RO" : "RW") << "] = 0x" << fillzero(8) << outhex(iter->second.csr->read()) << std::endl;
					out_list.emplace_back(std::pair<std::string, std::string>(iter->second.csr->get_name(), stream.str()));
				}

				std::sort(out_list.begin(), out_list.end(), csr_out_list_cmp);

				for(auto iter = out_list.begin();iter != out_list.end();iter++)
				{
					std::cout << iter->second;
				}
			}

			std::string get_info_packet()
			{
				std::stringstream result;

				std::vector<std::pair<std::string, std::string>> out_list;

				for(auto iter = csr_map_table.begin();iter != csr_map_table.end();iter++)
				{
					std::ostringstream stream;
					stream << outhex(iter->second.csr->read());
					out_list.emplace_back(std::pair<std::string, std::string>(iter->second.csr->get_name(), stream.str()));
				}

				std::sort(out_list.begin(), out_list.end(), csr_out_list_cmp);

				for(auto iter = out_list.begin();iter != out_list.end();iter++)
				{
					result << iter->first << ":" << iter->second;

					if(iter != out_list.end())
					{
						result << ",";
					}
				}

				return result.str();
			}
	};
}