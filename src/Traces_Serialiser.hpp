/*
 *    ████████╗██████╗  █████╗  ██████╗███████╗███████╗
 *    ╚══██╔══╝██╔══██╗██╔══██╗██╔════╝██╔════╝██╔════╝ A tool for saving side
 *       ██║   ██████╔╝███████║██║     █████╗  ███████╗ channel traces
 *       ██║   ██╔══██╗██╔══██║██║     ██╔══╝  ╚════██║
 *       ██║   ██║  ██║██║  ██║╚██████╗███████╗███████║ License : AGPLv3+
 *       ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝╚══════╝╚══════╝
 *
 *    ███████╗███████╗██████╗ ██╗ █████╗ ██╗     ██╗███████╗███████╗██████╗
 *    ██╔════╝██╔════╝██╔══██╗██║██╔══██╗██║     ██║██╔════╝██╔════╝██╔══██╗
 *    ███████╗█████╗  ██████╔╝██║███████║██║     ██║███████╗█████╗  ██████╔╝
 *    ╚════██║██╔══╝  ██╔══██╗██║██╔══██║██║     ██║╚════██║██╔══╝  ██╔══██╗
 *    ███████║███████╗██║  ██║██║██║  ██║███████╗██║███████║███████╗██║  ██║
 *    ╚══════╝╚══════╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝╚══════╝╚═╝╚══════╝╚══════╝╚═╝  ╚═╝
 *
 *    https://github.com/bristol-sca/Traces-Serialiser
 */

/*
 *  This file is part of Traces-Serialiser.
 *
 *  Traces-Serialiser is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Affero General Public License as published by the
 *  Free Software Foundation, either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  Traces-Serialiser is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with Traces-Serialiser.  If not, see <http://www.gnu.org/licenses/>.
 */

/*!
 *  @file Traces_Serialiser.hpp
 *  @brief This file contains the logic behind Traces_Serialiser. This is the
 *  only file needed to use this tool.
 *  @author Scott Egerton
 *  @date 2018
 *  @copyright GNU Affero General Public License Version 3+
 */

#ifndef SRC_TRACES_SERIALISER_HPP
#define SRC_TRACES_SERIALISER_HPP

#include <algorithm>    // for remove_if
#include <cstddef>      // for byte
#include <cstdint>      // for uint8_t, uint32_t
#include <fstream>      // for ofstream
#include <iomanip>      // for setw, setfill
#include <ios>          // for failure
#include <map>          // for map
#include <sstream>      // for ostringstream
#include <stdexcept>    // for range_error
#include <string>       // for string
#include <type_traits>  // for is_arithmetic, is_floating_point, is_same
#include <utility>      // for move, pair
#include <vector>       // for vector

namespace Traces_Serialiser
{
//! @class Serialiser
//! @brief This is the main class that is used in order to serialise traces.
//! Currently it supports saving in the format used by Riscure's inspector
//! tool.
//! @see https://www.riscure.com/security-tools/inspector-sca/
template <typename T_Sample = float> class Serialiser
{
    // Ensure that the template type T_Sample is arithmetic.
    static_assert(std::is_arithmetic<T_Sample>::value,
                  "Traces must be stored as a number");

private:
    //! This is the main container that stores the trace header information,
    //! ready to be saved into the output file. The format uses a
    //! type-length-value encoding to store this information.
    //! @see https://en.wikipedia.org/wiki/Type-length-value
    //! In this data structure, the headers are indexed by their type in a
    //! map. The map then contains a pair, which corresponds to the length
    //! and the value. The tag and length are stored as one or more bytes
    //! (std::vector<std::byte>).
    std::map<std::uint8_t,
             std::pair<std::vector<std::byte>, std::vector<std::byte>>>
        m_headers;

    //! @todo Document
    std::uint64_t
        m_number_of_traces;  //!@todo Does this need to be stored? - no -
                             //! m_traces.size() - Maybe use a macro instead ?
    std::uint64_t
        m_samples_per_trace;  //!@todo Does this need to be stored? - no -
                              //! m_traces.size() - Maybe use a macro
                              //! instead?
    const std::uint8_t m_sample_length;

    std::size_t m_longest_trace_length;

    std::vector<std::string> m_extra_data;

    //! This contains the actual side channel analysis traces, stored as
    //! bytes ready to be saved into the output file.
    //! @todo Don't store a traces object. This is simply the value to the
    //! Tag_Trace_Block_Marker.
    std::vector<std::vector<T_Sample>> m_traces;

    //! @brief Converts the data given by the parameter p_data into a series
    //! of bytes.
    //! @param p_data The data to be converted to bytes. This uses templates
    //! so that this function can convert any basic data type and
    //! std::string to bytes.
    //! @returns A series of bytes represented using std::vector<std::byte>.
    //! @todo See if this can all be replaced by std::bitset?
    template <typename T_Data>
    static const std::vector<std::byte> convert_to_bytes(const T_Data& p_data)
    {
        // A temporary store for the converted bytes.
        std::vector<std::byte> bytes_vector;

        // Strings need to be handled separately.
        if constexpr (std::is_same<T_Data, std::string>::value)
        {
            // Get a char array from the string and cast it to a byte array
            // TODO: std::string is a container. Treat it as such using
            // convert_traces_to_bytes() instead of separate handling here.
            auto bytes_array =
                reinterpret_cast<const std::byte*>(p_data.c_str());

            // Convert byte array to byte vector
            bytes_vector = {bytes_array, bytes_array + p_data.size()};
        }
        else
        {
            // Cast to a byte array
            auto bytes_array = reinterpret_cast<const std::byte*>(&p_data);

            // Convert byte array to byte vector
            bytes_vector = {bytes_array, bytes_array + sizeof(T_Data)};

            // TODO:: Should this affect floats only? or strings too?
            if constexpr (std::is_integral<T_Data>::value)
            {
                // Needed to remove trailing 0s
                bytes_vector.erase(
                    std::remove_if(bytes_vector.begin(),
                                   bytes_vector.end(),
                                   [](const std::byte byte) {
                                       return 0 ==
                                              std::to_integer<uint8_t>(byte);
                                   }),
                    bytes_vector.end());

                // If bytes_vector is empty then removing trailing 0s has
                // removed the original value, 0; therefore re-add it.
                if (bytes_vector.empty())
                {
                    bytes_vector.push_back(std::byte{0});
                }
            }
        }
        return bytes_vector;
    }

    //! @brief This function is intended to ensure that each item in p_data
    //! correct length as defined by p_sample_length by padding it with 0s.
    //! This function will prepend 0s onto the front of p_data.
    //! If the length of the data is already greater than or equal to
    //! p_sample_length, no action will be taken. This may result in implicit
    //! shortening of the value but most modern compilers will pick this up.
    //! @tparam T The type of the data to the padded. The type stored in p_data.
    //! This will work with any type where T{0} will successfully construct.
    //! @param p_data The data to be padded to the correct length as a
    //! vector of bytes.
    //! @param p_length The length the data will be padded to.
    //! @return Returns a copy of the original data vector, padded with 0s to
    //! the length given by p_sample_length.
    //! @todo Pass p_data by pointer
    template <typename T>
    static const std::vector<T> pad_front(std::vector<T> p_data,
                                          const std::uint8_t p_length)
    {
        while (p_data.size() < p_length)
        {
            p_data.insert(std::begin(p_data), T{0});
        }
        return p_data;
    }

    //! @brief This function is intended to ensure that each item in p_data
    //! correct length as defined by p_sample_length by padding it with 0s.
    //! This function will append 0s onto the end of p_data.
    //! If the length of the data is already greater than or equal to
    //! p_sample_length, no action will be taken. This may result in implicit
    //! shortening of the value but most modern compilers will pick this up.
    //! @tparam T The type of the data to the padded. The type stored in p_data.
    //! This will work with any type where T{0} will successfully construct.
    //! @param p_data The data to be padded to the correct length as a
    //! vector of bytes.
    //! @param p_length The length the data will be padded to.
    //! @return Returns a copy of the original data vector, padded with 0s to
    //! the length given by p_sample_length.
    //! @todo Pass p_data by pointer
    template <typename T>
    static const std::vector<T> pad_back(std::vector<T> p_data,
                                         const std::size_t p_length)
    {
        while (p_data.size() < p_length)
        {
            p_data.emplace_back(0);
        }
        return p_data;
    }

    //! @brief Retrieves the length of the longest element in p_data.
    //! @param p_data The contain from which the length of the longest element
    //! will be found.
    //! @tparam T The type of the data to the padded. The type stored in p_data.
    //! This will work with any type where the operator < is defined.
    //! @returns The size of the largest element in p_data
    template <typename T> static std::size_t get_longest(const T& p_data)
    {
        return (*std::max_element(
                    std::begin(p_data),
                    std::end(p_data),
                    [](const auto& p_first, const auto& p_second) {
                        return p_first.size() < p_second.size();
                    }))
            .size();
    }

    //! @brief Pads all of the data in m_traces to ensure each trace is of the
    //! same length using pad_back().
    //! This is necessary as TRS files require all traces to be of the same
    //! length.
    void pad_all_traces()
    {
        // If the longest trace length is not yet recorded, so calculate it.
        if (0 == m_longest_trace_length)
        {
            m_longest_trace_length = get_longest(m_traces);
        }
        for (auto& trace : m_traces)
        {
            trace = pad_back(trace, m_longest_trace_length);
        }

        // This may have changed during padding.
        m_samples_per_trace = m_traces.front().size();
    }

    //! @brief Converts a vector of data given by the parameter p_data into a
    //! single series of bytes.
    //! The input can be a nested vector, in which case it will be recursively
    //! unpacked it and the values inside will be converted to bytes.
    //! @param p_data A vector or nested vector contained the data to be
    //! converted to bytes. This uses templates so that this function can
    //! convert any basic data type and std::string to bytes.
    //! @param p_sample_length The length each sample should be. This is used to
    //! pad each individual sample to the correct length.
    //! @returns A series of bytes represented using std::vector<std::byte>.
    template <typename T_Traces>
    static const std::vector<std::byte>
    convert_traces_to_bytes(const std::vector<T_Traces>& p_data,
                            const std::uint8_t p_sample_length)
    {
        std::vector<std::byte> bytes_vector;

        // For each value in the vector
        for (const auto& data : p_data)
        {
            const std::vector<std::byte> data_vector{[&]() {
                // If this is nested container, recursively unpack it until
                // we get at the values inside.
                if constexpr (!std::is_same<std::vector<T_Traces>,
                                            std::vector<T_Sample>>::value)
                {
                    // Check that each sub container is of the same length

                    return convert_traces_to_bytes(data, p_sample_length);
                }
                // if this is not a nested container simply convert
                // each of the values.
                return pad_front(convert_to_bytes(data), p_sample_length);
            }()};

            // Append the converted values onto the end of bytes_vector
            // ready to be returned.
            bytes_vector.insert(std::end(bytes_vector),
                                std::begin(data_vector),
                                std::end(data_vector));
        }
        return bytes_vector;
    }

    //! @brief A helper function to add all of the headers required to be in
    //! a trace file. The headers currently required are the number of
    //! traces, the number of samples per trace and the sample coding. The
    //! sample coding is not required as an input here as it can be
    //! calculated based off of the length of one sample.
    //! @note Although Tag_Trace_Block_Marker (0x5F) is a required header,
    //! it is not included here as it needs to be the last header printed
    //! before the traces.
    //! @param p_number_of_traces The total number of traces.
    //! @param p_samples_per_trace The number of samples within each trace.
    //! @param p_sample_length The length of a single sample in bytes.
    void add_required_headers(const std::uint32_t p_number_of_traces,
                              const std::uint32_t p_samples_per_trace,
                              const std::uint8_t p_sample_length)
    {
        validate_required_headers(
            p_number_of_traces, p_samples_per_trace, p_sample_length);

        Add_Header(Tag_Number_Of_Traces, p_number_of_traces);
        Add_Header(Tag_Number_Of_Samples_Per_Trace, p_samples_per_trace);

        // Calculate the sample coding.
        // Bits 8-6 are reserved and must be '000'.
        // Bit 5 corresponds to integer (0) or floating point (1).
        // Bits 4-1 are the sample length in bytes. This must be 1,2 or 4.
        const std::uint8_t sample_coding{[&p_sample_length]() -> std::uint8_t {
            // If the traces are floating point values, set bit 5 to
            // indicate
            // this as per the Riscure inspector specification: Table K.2.
            // Sample coding.
            if constexpr (std::is_floating_point<T_Sample>::value)
            {
                return p_sample_length | 0b10000;
            }
            // If the traces are not floating point then the sample coding is
            // simply the length of one sample.
            return p_sample_length;
        }()};

        Add_Header(Tag_Sample_Coding, sample_coding);
    }

    //! @brief Ensures that all of the required headers are valid. This does not
    //! return anything as an exception will be thrown if the validation fails.
    //! Checks that the sample length is 1, 2 or 4 bytes.
    //! Checks that the headers indicating the various lengths and sizes are
    //! correct by checking the amount of traces provided
    //! @param p_number_of_traces The total number of traces.
    //! @param p_samples_per_trace The number of samples within each trace.
    //! @param p_sample_length The length of a single sample in bytes.
    //! @exception std::range_error If the sample length is an invalid value
    //! then this exception will be thrown.
    //! @exception std::domain_error If the sizes and lengths do not match what
    //! is in m_traces then this is thrown.
    constexpr void
    validate_required_headers(const std::uint32_t p_number_of_traces,
                              const std::uint32_t p_samples_per_trace,
                              const std::uint8_t p_sample_length)
    {
        if (4 < p_sample_length || 3 == p_sample_length)
        {
            throw std::range_error("Sample length must be either 1, 2 or 4");
        }

        if (p_number_of_traces * p_samples_per_trace !=
            m_traces.size() * m_traces.front().size())
        {
            throw std::domain_error(
                "Invalid parameters given. Either the number of traces, number "
                "of samples per trace or the sample length is incorrect.");
        }
    }

    //! @brief Checks if all of the extra data within p_extra_data of the
    //! same length. This does not return anything as an exception will be
    //! thrown if the validation fails.
    //! @param p_extra_data The data to be checked
    //! @exception std::domain_error If they are not all the same length
    //! then this exception is thrown.
    template <typename T_Traces>
    static constexpr void
    validate_extra_data_length(const std::vector<T_Traces>& p_extra_data)
    {
        if (!check_all_same_length(p_extra_data))
        {
            throw std::domain_error("Extra data must all be the same length");
        }
    }

    template <typename T>
    static constexpr bool check_all_same_length(const std::vector<T>& p_data)
    {
        // If everything in p_data is not the same length
        //
        // Finds the first occurrence of two adjacent data points that don't
        // have the same size. If this is the end of the vector then none were
        // found and they are all the same size.
        return std::adjacent_find(std::begin(p_data),
                                  std::end(p_data),
                                  [](const T& p_data_1, const T& p_data_2) {
                                      // Check if these two are the same size.
                                      return p_data_1.size() != p_data_2.size();
                                  }) == std::end(p_data);
    }

    //! @brief Ensures that setting the header given by the parameter p_tag
    //! is allowed in the current context, based on which headers have
    //! already been set.
    //! @param p_tag The tag indicating which header is currently being set.
    //! @exception std::range_error This does not return anything as an
    //! exception will be thrown if the validation fails.
    constexpr void validate_header(const std::uint8_t p_tag)
    {
        // Only allow external clock related values to be set if the
        // external clock has been explicitly enabled.

        // If this is not an external clock related header then no
        // validation is required.
        if (!is_external_clock_header(p_tag))
        {
            return;
        }

        // Tag_External_Clock_Used must be set before setting any other
        // Tag_External_Clock_* headers
        if (!header_enabled(Tag_External_Clock_Used))
        {
            throw std::range_error("Enable external clock explicitly with "
                                   "Set_External_Clock_Used()");
        }

        // Only allow external clock resampler mask to be set if the
        // external clock has been explicitly enabled.

        // If this is not an external clock resampler mask related header
        // then no further validation is required.
        if (Tag_External_Clock_Resampler_Mask != p_tag)
        {
            return;
        }

        // Tag_External_Clock_Resampler_Enabled must be set before setting
        // Tag_External_Clock_Resampler_Mask
        if (!header_enabled(Tag_External_Clock_Resampler_Enabled))
        {
            throw std::range_error(
                "Enable external clock resampler explicitly with "
                "Set_External_Clock_Resampler_Enabled()");
        }
    }

    //! @brief Determines whether or not the given boolean header has been
    //! enabled or not.
    //! @returns If the header has been set to a value other than 0, true is
    //! return. If it is unset or set to 0 then false is returned.
    //! @param p_tag The tag indicating which header to check.
    //! @warning This was designed for headers with boolean values but it
    //! can be used on any header which will usually give undesired results.
    constexpr bool header_enabled(const std::uint8_t p_tag) const
    {
        // If the header has not been set then it is not enabled
        if (m_headers.end() == m_headers.find(p_tag))
        {
            return false;
        }

        // If the header has been set to 0 then it is not enabled
        return 0 != std::to_integer<bool>(m_headers.at(p_tag).second.front());
    }

    //! @brief Determines whether or not the tag given by p_tag is related to
    //! the external clock. All headers related to the external clock lie
    //! between 0x61 (Tag_External_Clock_Threshold) and 0x67
    //! (Tag_External_Clock_Time_Base) inclusive. Expect for
    //! Tag_External_Clock_Used (0x60) which is considered an exception as
    //! it enables this range of tags to be used.
    //! @warning This will return false for 0x60 (Tag_External_Clock_Used)
    //! :param p_tag The tag to be checked.
    //! @returns True if p_tag is an external clock header and false if not.
    constexpr static bool is_external_clock_header(const std::uint8_t p_tag)
    {
        return Tag_External_Clock_Threshold <= p_tag &&
               Tag_External_Clock_Time_Base >= p_tag;
    }

    //! @todo: document
    void save_headers(std::ofstream& m_output_file) const
    {
        // TODO: If all of the samples are smaller than the sample length
        // then the sample length can be reduced, saving a lot of file size.

        // Output each header
        // // TODO: Split this into multiple functions
        for (const auto& header : m_headers)
        {
            // Output tag
            m_output_file << header.first;

            // Output length
            for (const auto& length_byte : header.second.first)
            {
                m_output_file << std::to_integer<uint8_t>(length_byte);
            }

            // Output value
            for (const auto& value_byte : header.second.second)
            {
                m_output_file << std::to_integer<uint8_t>(value_byte);
            }
        }

        // The start of traces is marked by a Trace Block Marker tag.
        m_output_file << Tag_Trace_Block_Marker;

        // The length of the Trace Block Marker (always 0) is still
        // required.
        m_output_file.put(0x00);
    }

    bool is_extra_data_digits() const
    {
        // Skip printing extra data if there is none. TODO: Is this even
        // needed?
        if (0 == m_extra_data.size())
        {
            return false;
        }

        // If this is completely numerical then output it as raw
        // numbers, not ACSII.
        return std::all_of(std::begin(m_extra_data),
                           std::end(m_extra_data),
                           [](const std::string& string) {
                               return std::all_of(std::begin(string),
                                                  std::end(string),
                                                  ::isxdigit);
                           });
    }

    //! @todo: document
    void save_extra_data(std::ofstream& m_output_file,
                         const std::size_t p_index,
                         const bool m_is_digits) const
    {
        // Skip printing extra data if there is none. TODO: Is this even
        // needed?
        if (0 == m_extra_data.size())
        {
            return;
        }

        // If this is completely numerical then output it as raw
        // numbers, not ACSII.
        if (m_is_digits)
        {
            // Increment by 2 at a time as 2 characters fit into a byte.
            for (std::size_t i{0}, size{m_extra_data[p_index].length() - 1};
                 i < size;
                 i += 2)
            {
                // Get two characters from the string as 2 character make up
                // 1 byte. Convert them to a number using std::stoi() and
                // then convert the result to a byte using
                // convert_to_bytes(). As convert_to_bytes() returns a
                // vector but in this case it will only contain one byte,
                // .front() is used to retrieve it from the vector. This is
                // then converted to a std::uint8_t to allow the << operator
                // to stream the result.
                m_output_file << std::to_integer<std::uint8_t>(
                    convert_to_bytes(
                        std::stoi(m_extra_data[p_index].substr(i, 2).c_str(),
                                  nullptr,
                                  16))
                        .front());
            }
        }
        else
        {
            for (const auto& character : m_extra_data[p_index])
            {
                m_output_file << character;
            }
        }
    }

    //! @todo: document
    void save_trace(std::ofstream& m_output_file,
                    const std::size_t p_index) const
    {
        for (const auto& sample :
             convert_traces_to_bytes(m_traces[p_index], m_sample_length))
        {
            m_output_file << std::to_integer<std::uint8_t>(sample);
        }
    }

public:
    // These variables are intended to improve readability and nothing more.
    // Public so user can write code like this:
    // Add_Header(Tag_Number_Of_Traces, 4);
    // clang-format off
    constexpr static std::uint8_t Tag_Number_Of_Traces                 {0x41};
    constexpr static std::uint8_t Tag_Number_Of_Samples_Per_Trace      {0x42};
    constexpr static std::uint8_t Tag_Sample_Coding                    {0x43};
    constexpr static std::uint8_t Tag_Length_Of_Cryptographic_Data     {0x44};
    constexpr static std::uint8_t Tag_Title_Space_Per_Trace            {0x45};
    constexpr static std::uint8_t Tag_Trace_Title                      {0x46};
    constexpr static std::uint8_t Tag_Description                      {0x47};
    constexpr static std::uint8_t Tag_Axis_Offset_X                    {0x48};
    constexpr static std::uint8_t Tag_Axis_Label_X                     {0x49};
    constexpr static std::uint8_t Tag_Axis_Label_Y                     {0x4A};
    constexpr static std::uint8_t Tag_Axis_Scale_X                     {0x4B};
    constexpr static std::uint8_t Tag_Axis_Scale_Y                     {0x4C};
    constexpr static std::uint8_t Tag_Trace_Offset                     {0x4D};
    constexpr static std::uint8_t Tag_Logarithmic_Scale                {0x4E};
    // 0x4F - 0x54 Reserved for future use.
    constexpr static std::uint8_t Tag_Scope_Range                      {0x55};
    constexpr static std::uint8_t Tag_Scope_Coupling                   {0x56};
    constexpr static std::uint8_t Tag_Scope_Offset                     {0x57};
    constexpr static std::uint8_t Tag_Scope_Input_Impedance            {0x58};
    constexpr static std::uint8_t Tag_Scope_ID                         {0x59};
    constexpr static std::uint8_t Tag_Filter_Type                      {0x5A};
    constexpr static std::uint8_t Tag_Filter_Frequency                 {0x5B};
    constexpr static std::uint8_t Tag_Filter_Range                     {0x5C};
    // 0x5D - 0x5E Undocumented.
    constexpr static std::uint8_t Tag_Trace_Block_Marker               {0x5F};
    constexpr static std::uint8_t Tag_External_Clock_Used              {0x60};
    constexpr static std::uint8_t Tag_External_Clock_Threshold         {0x61};
    constexpr static std::uint8_t Tag_External_Clock_Multiplier        {0x62};
    constexpr static std::uint8_t Tag_External_Clock_Phase_Shift       {0x63};
    constexpr static std::uint8_t Tag_External_Clock_Resampler_Mask    {0x64};
    constexpr static std::uint8_t Tag_External_Clock_Resampler_Enabled {0x65};
    constexpr static std::uint8_t Tag_External_Clock_Frequency         {0x66};
    constexpr static std::uint8_t Tag_External_Clock_Time_Base         {0x67};
    // clang-format on

    //! @brief Constructs the Serialiser object and adds all of the
    //! mandatory data. Optional headers can be set later. All traces are
    //! required to be passed to the constructor.
    //! @param p_traces All of the traces stored as 2D vector. This is
    //! interpreted as a vector of traces with each trace being a vector of
    //! samples.
    //! @param p_sample_length The length of a trace sample in bytes. This
    //! can optionally be specified. If not specified then it is assumed to
    //! be the size of the data type samples are stored as, given by
    //! T_Sample.
    // TODO: Add support for cryptographic data to be included in each
    // trace.
    Serialiser(const std::vector<std::vector<T_Sample>>& p_traces,
               const std::uint8_t p_sample_length = sizeof(T_Sample))
        : Serialiser{{}, p_traces, p_sample_length}
    {
    }

    //! @todo Document
    //! @todo Does p_sample_length ever need to be specified manually? It
    //! certainly does not for 2d constructors.
    Serialiser(const std::vector<std::string>& p_extra_data,
               const std::vector<std::vector<T_Sample>>& p_traces,
               const std::uint8_t p_sample_length = sizeof(T_Sample))
        : m_headers{}, m_number_of_traces{p_traces.size()},
          // Number of samples per trace can be assumed to be the length of
          // one trace.
          m_samples_per_trace{p_traces.front().size()},
          m_sample_length{p_sample_length},
          // Set longest trace length to 0 for now. It will be changed later
          m_longest_trace_length{0}, m_extra_data{p_extra_data}, m_traces{
                                                                     p_traces}
    {
    }

    //! @todo Document
    explicit Serialiser(const std::uint8_t p_sample_length = sizeof(T_Sample))
        : Serialiser{{{}}, p_sample_length}
    {
    }

    //! @brief This appends a single trace to the end of the list of traces.
    //! Extra data associated with this trace can also be added using
    //! p_extra_data. This will also validate the length of this data and
    //! can throw exceptions if this is of the incorrect length.
    //! @param p_trace The trace to be added.
    //! @param p_extra_data The extra data with this trace to be added.
    void Add_Trace(const std::vector<T_Sample>& p_trace,
                   const std::string& p_extra_data = std::string{})
    {
        // If this is the first trace provided then m_samples_per_trace needs to
        // be set.
        // m_traces can contain 0 as the first element as a side effect of
        // initialisation.
        if (m_traces.front().empty())
        {
            m_samples_per_trace = p_trace.size();
            // If this was an empty trace set, m_traces can contain 0 as the
            // first element.
            m_traces[0] = p_trace;

            // Reset m_number_of_traces as this is the first element. This will
            // be incremented shortly.
            m_number_of_traces = 0;
        }
        else
        {
            m_traces.emplace_back(p_trace);
        }

        if (!p_extra_data.empty())
        {
            m_extra_data.emplace_back(p_extra_data);
        }

        // TODO: Does this need to be stored?
        m_number_of_traces++;
    }

    //! @brief This is this function that adds headers to the list of
    //! headers to be saved. This is called by all other functions that add
    //! headers as it is the only place headers are added.
    //! @param p_tag The tag representing which header is currently being
    //! set.
    //! @param p_data The data that should be assigned to the header given
    //! by p_tag.
    //! @note This is public to allow user to add new headers that may not
    //! have functions
    template <typename T_Data>
    void Add_Header(const std::uint8_t& p_tag, const T_Data& p_data)
    {
        // TODO: Handle case where bit 8 (msb) is set to '0' in object
        // length. See inspector manual for details.

        validate_header(p_tag);

        // A temporary variable to convert p_data to bytes.
        const std::vector<std::byte> value{convert_to_bytes(p_data)};

        std::vector<std::byte> length = convert_to_bytes(value.size());

        // If the length doesn't fit into 7 bits then the 8th bit is set
        // indicating that more than one byte is used to store the length
        // and that the first byte is the length of the length.
        if (0b01111111 < value.size())
        {
            // TODO: If the length is longer than 65025 bytes (65KB) then
            // the resulting file will be incorrect. Does this need to be
            // accounted for?
            length.insert(length.begin(),
                          std::byte(0b10000000 | length.size()));
        }

        // Add it to the map of headers.
        m_headers[p_tag] = std::make_pair(length, value);
    }

    //! @brief This saves the current state of the headers, along with the
    //! traces to a file specified by p_file_path.
    //! @param p_file_path The path of the file to save to.
    //! @note If the path contains a directory that doesn't exist, it will
    //! not be created, instead an error file be thrown. New files will be
    //! created however.
    //! @exception std::ios_base::failure Throws an exception if creating
    //! the output stream fails for any reason. For example, directory
    //! doesn't exist.
    void Save(const std::string& p_file_path)
    {
        std::ofstream output_file(p_file_path,
                                  std::ios::out | std::ios::binary);

        if (!output_file)
        {
            throw std::ios_base::failure("An error occurred when preparing "
                                         "the file to be written to");
        }

        // TRS files require all traces to be of the same length.
        pad_all_traces();

        bool is_digits{false};

        // Set this header to match the data that is stored.
        //! @todo this will override any user set value. Maybe make this a
        //! private function as a solution?
        if (m_extra_data.size() > 0)  // Don't do this if not extra data is
                                      // supplied, it will cause a Segfault.
        {
            Set_Cryptographic_Data_Length(m_extra_data.front().size());
            is_digits = is_extra_data_digits();

            if (is_digits)
            {
                // Digits take up half the space of ASCII.
                // So retrieve the current value, half it and save it back.
                Add_Header(Tag_Length_Of_Cryptographic_Data,
                           std::to_integer<std::uint8_t>(
                               m_headers[Tag_Length_Of_Cryptographic_Data]
                                   .second.front()) /
                               2);
            }
        }

        // Ensure information stored will create a valid trs file.
        //! @todo Group all THREE validation functions in a valid function.
        validate_extra_data_length(m_extra_data);

        // This has to be done after changing the length of cryptographic
        // data as it is dependant on that information.
        add_required_headers(
            m_number_of_traces, m_samples_per_trace, m_sample_length);

        save_headers(output_file);

        // For each trace
        {
            const std::size_t size{m_traces.size()};
            for (std::size_t i{0}; i < size; ++i)
            {
                save_extra_data(output_file, i, is_digits);
                save_trace(output_file, i);
            }
        }

        output_file.close();
    }

    // Beyond this point there are only functions designed to simplify the
    // usage of the Add_Header function.
    // The default parameters in the following functions are copied from the
    // Riscure inspector documentation.

    // TODO: Rename
    // https://wandbox.org/permlink/TiG2k4xzQUojzfdO
    void Set_Cryptographic_Data_Length(const std::uint16_t p_length = 0)
    {
        Add_Header(Tag_Length_Of_Cryptographic_Data, p_length);
    }

    // TODO: Implement this.
    void Set_Title_Space_Per_Trace(const std::uint8_t p_length = 0)
    {
        Add_Header(Tag_Title_Space_Per_Trace, p_length);
    }

    void Set_Trace_Title(const std::string& p_title = "trace")
    {
        Add_Header(Tag_Trace_Title, p_title);
    }

    void Set_Trace_Description(const std::string& p_description)
    {
        Add_Header(Tag_Description, p_description);
    }

    void Set_Axis_Offset_X(const std::uint32_t p_offset = 0)
    {
        Add_Header(Tag_Axis_Offset_X, p_offset);
    }

    void Set_Axis_Label_X(const std::string& p_label)
    {
        Add_Header(Tag_Axis_Label_X, p_label);
    }

    void Set_Axis_Label_Y(const std::string& p_label)
    {
        Add_Header(Tag_Axis_Label_Y, p_label);
    }

    void Set_Axis_Scale_X(const float p_scale = 1)
    {
        Add_Header(Tag_Axis_Scale_X, p_scale);
    }

    void Set_Axis_Scale_Y(const float p_scale = 1)
    {
        Add_Header(Tag_Axis_Scale_Y, p_scale);
    }

    void Set_Trace_Offset(const std::uint32_t p_offset = 0)
    {
        Add_Header(Tag_Trace_Offset, p_offset);
    }

    // TODO: Calling this with a float argument works. Validate to ensure
    // that this does not work.
    void Set_Logarithmic_Scale(const std::uint8_t p_scale = 0)
    {
        Add_Header(Tag_Logarithmic_Scale, p_scale);
    }

    // 0x4F - 0x54 Reserved for future use.

    void Set_Scope_Range(const float p_range = 0)
    {
        Add_Header(Tag_Scope_Range, p_range);
    }

    void Set_Scope_Coupling(const std::uint32_t p_coupling = 0)
    {
        Add_Header(Tag_Scope_Coupling, p_coupling);
    }

    void Set_Scope_Offset(const float p_offset = 0)
    {
        Add_Header(Tag_Scope_Offset, p_offset);
    }

    void Set_Scope_Input_Impedance(const float p_impedance = 0)
    {
        Add_Header(Tag_Scope_Input_Impedance, p_impedance);
    }

    // TODO: Should this be a string?
    void Set_Scope_ID(const std::string& p_id)
    {
        Add_Header(Tag_Scope_ID, p_id);
    }

    void Set_Filter_Type(const std::uint32_t p_type = 0)
    {
        Add_Header(Tag_Filter_Type, p_type);
    }

    void Set_Filter_Frequency(const float p_frequency = 0)
    {
        Add_Header(Tag_Filter_Frequency, p_frequency);
    }

    void Set_Filter_Range(const float p_range = 0)
    {
        Add_Header(Tag_Filter_Range, p_range);
    }

    // 0x5D - 0x5E Undocumented
    // 0x5F Marks header end

    void Set_External_Clock_Used(const bool p_used = true)
    {
        Add_Header(Tag_External_Clock_Used, p_used);
    }

    void Set_External_Clock_Threshold(const float p_threshold = 0)
    {
        Add_Header(Tag_External_Clock_Threshold, p_threshold);
    }

    void Set_External_Clock_Multiplier(const std::uint32_t p_multiplier = 0)
    {
        Add_Header(Tag_External_Clock_Multiplier, p_multiplier);
    }

    void Set_External_Clock_Phase_Shift(const std::uint32_t p_phase_shift = 0)
    {
        Add_Header(Tag_External_Clock_Phase_Shift, p_phase_shift);
    }

    void
    Set_External_Clock_Resampler_Mask(const std::uint32_t p_resampler_mask = 0)
    {
        Add_Header(Tag_External_Clock_Resampler_Mask, p_resampler_mask);
    }

    void
    Set_External_Clock_Resampler_Enabled(const bool p_resampler_enabled = true)
    {
        Add_Header(Tag_External_Clock_Resampler_Enabled, p_resampler_enabled);
    }

    void Set_External_Clock_Frequency(const float p_frequency = 0)
    {
        Add_Header(Tag_External_Clock_Frequency, p_frequency);
    }

    // TODO: All int headers are unsigned, Change them to signed?
    void Set_External_Clock_Time_Base(const std::uint32_t p_time_base = 0)
    {
        Add_Header(Tag_External_Clock_Time_Base, p_time_base);
    }
};
}  // namespace Traces_Serialiser
#endif  // SRC_TRACES_SERIALISER_HPP
