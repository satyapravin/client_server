#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <streambuf>
#include <istream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <list>
#include <queue>
#include <algorithm>
#include <fstream>

namespace agpc {

    // family represents parent and all its child records
    class family {
    public:
        void push_back(std::string &record) {
            records_.push_back(std::move(record));
        }

        int32_t size() const { return records_.size(); }

        std::vector<std::string> get_records() {
            return records_;
        }

    protected:
        // its bad choice, in production code I would use
        // a pool based list to avoid allocation and still be cache friendly.
        // but in interest of time keeping its simple here.
        std::vector<std::string> records_;

    };

    // comparator to use family class in stl containers
    class family_size_comparator {
    public:
        bool operator()(family *lsh, family *rhs) {
            return lsh->size() < rhs->size();
        }
    };

    // cluster represends a group that has more than minimum records with the constraint
    // that parent and all its children will always be in same cluster
    /// this is comparator class for keep clusters sorted in stl containers.

    class cluster_size_comparator {
    public:
        bool operator()(std::vector<family *> &lsh, std::vector<family *> &rhs) {
            int32_t lsize = 0;
            int32_t rsize = 0;

            for (family *f : lsh) {
                lsize += f->size();
            }

            for (family *f : rhs) {
                rsize += f->size();
            }

            return lsize < rsize;
        }
    };

    // main manager class
    // builds parent child relationship as it gets callback from mmap reader class
    class family_manager {
    public:

        // gets callback from mmap reader class
        // keeps logical grouping of records with parent child in same family
        void on_record(std::string &record) {

            if (record.length() > 0) {
                // no need to tokenize entire record.
                // interesting information is either at beginning or end

                char type = record[0];
                int32_t id = 0;
                int32_t parent_id = 0;

                std::size_t found = record.rfind(',');
                if (found != std::string::npos) {
                    parent_id = std::stoi(record.substr(found + 1, std::string::npos));
                } else {
                    // bad record in file... log error handling.
                    return;
                }

                std::size_t found2 = record.rfind(',', found - 1);

                // little bit of hackery to avoid looking into string too deep.
                if (found2 != std::string::npos) {
                    size_t len = found - found2 - 1;
                    id = std::stoi(record.substr(found2 + 1, len));
                }

                if (type == 'T') {
                    family *grp = new family();
                    // assuming no duplicate parent records
                    // don't have time to do much error checking.
                    grp->push_back(record);
                    groups_[id] = grp;

                    // if there are any pending children of this parent clean them
                    if (pending_.find(parent_id) != pending_.end()) {
                        std::list<std::string> &children = pending_[parent_id];
                        for (std::string &record : children) {
                            grp->push_back(record);
                        }
                    }
                } else if (type == 'P') {
                    if (groups_.find(parent_id) != groups_.end()) {
                        family *parent = groups_[parent_id];
                        parent->push_back(record);
                    } else // parent is not there yet, put in pendings
                    {
                        if (pending_.find(parent_id) != pending_.end()) {
                            pending_[parent_id].push_back(std::move(record));
                        } else {
                            std::list<std::string> records;
                            records.push_back(record);
                            pending_[parent_id] = std::move(records);
                        }
                    }
                }
            }
        }

        const std::unordered_map<int, family *> &get_groups() {
            return groups_;
        };

    protected:

        std::unordered_map<int, family *> groups_;
        std::unordered_map<int, std::list<std::string>> pending_;
    };

    // this class basically puts all families into clusters
    // uses greedy approach, (I guess there must be some better way like dynamic programming etc..)
    // but can't think of it right now.

    // greedy approach work like this.

    // all families with size greater than minimum go to their own groups
    // remaining families are sorted in decending order of theie size (max heap/priority queue)
    // we pick each family from max heap, and put it in cluster with least size.
    // in the end there could be some clusters with less than minimum so we merge them.

    class clusterizer {
    public:

        clusterizer(int min_members) : min_members_{min_members} {

        }

        void make_clusters(const std::unordered_map<int, family *> &all_familes) {
            int total_size = 0;
            for (auto x: all_familes) {
                priority_queue_family_.push(x.second);
                if (x.second->size() < min_members_) {
                    total_size += x.second->size();
                }
            }

            int max_rem_groups = total_size / min_members_;

            if (max_rem_groups == 0) {
                max_rem_groups += 1;
            }

            for (int i = 0; i < max_rem_groups; i++) {
                std::vector<family *> v;
                clusters_.push_back(std::move(v));
            }

            while (!priority_queue_family_.empty()) {
                family *f = priority_queue_family_.top();
                int size = f->size();

                if (size >= min_members_) {
                    big_clusters_.push_back(f);
                } else {
                    int i = find_min_cluster();
                    clusters_[i].push_back(f);
                }

                priority_queue_family_.pop();
            }

            if (total_records_in_cluster(clusters_[0]) == 0) {
                clusters_.clear();
                return;
            }
            std::sort(clusters_.begin(), clusters_.end(), cluster_size_comparator());


            merge_partial_clusters();

        }

        void write_files() {
            // all clusters build write them in files.

            int count = 1;

            for (auto bc : big_clusters_) {
                std::string filenname_token = "output_";
                filenname_token += std::to_string(count);
                count++;
                filenname_token += ".txt";

                std::ofstream outfile(filenname_token.c_str());
                for (auto arecord : bc->get_records()) {
                    outfile << arecord;
                    outfile << std::endl;
                }

                outfile.close();

            }

            for (auto mc : merged_clusters_) {
                std::string filenname_token = "output_";
                filenname_token += std::to_string(count);
                count++;
                filenname_token += ".txt";

                std::ofstream outfile(filenname_token.c_str());
                for (auto afamily : mc) {
                    for (auto arecord : afamily->get_records()) {
                        outfile << arecord;
                        outfile << std::endl;
                    }
                }

                outfile.close();
            }

            for (int i = start_partial_index_; i < clusters_.size(); i++) {
                std::string filenname_token = "output_";
                filenname_token += std::to_string(count);
                count++;
                filenname_token += ".txt";

                std::ofstream outfile(filenname_token.c_str());
                for (auto afamily : clusters_[i]) {
                    for (auto arecord : afamily->get_records()) {
                        outfile << arecord;
                        outfile << std::endl;
                    }
                }

                outfile.close();
            }

            if (partial_cluster_) {
                for (int i = start_partial_index_; i <= stop_partial_index_ && i < clusters_.size(); i++) {
                    std::string filenname_token = "output_";
                    filenname_token += std::to_string(count);
                    count++;
                    filenname_token += ".txt";

                    std::ofstream outfile(filenname_token.c_str());
                    for (auto afamily : clusters_[i]) {
                        for (auto arecord : afamily->get_records()) {
                            outfile << arecord;
                            outfile << std::endl;
                        }
                    }

                    outfile.close();
                }
            }
        }

    protected:

        int find_min_cluster() {
            int minindex = 0;
            int min_val = INT32_MAX;
            for (int i = 0; i < clusters_.size(); i++) {
                int total = total_records_in_cluster(clusters_[i]);

                if (total < min_val) {
                    min_val = total;
                    minindex = i;
                }
            }

            return minindex;
        }

        int total_records_in_cluster(std::vector<family *> &cluster) {
            int tsize = 0;
            for (auto x : cluster) {
                tsize += x->size();
            }
            return tsize;
        }

        void merge_partial_clusters() {
            int partial_sum = 0;
            int start_partial_sum = 0;
            int stop_partial_sum = 0;
            bool any_partial = true;

            for (int i = 0; i < clusters_.size(); i++) {
                if (total_records_in_cluster(clusters_[i]) >= min_members_) {
                    if (start_partial_sum == 0 && stop_partial_sum == 0) {
                        any_partial = false;
                    }
                    break;
                }

                partial_sum += total_records_in_cluster(clusters_[i]);
                stop_partial_sum = i;

                if (partial_sum >= min_members_) {


                    std::vector<family *> merged;
                    for (int j = start_partial_sum; j <= stop_partial_sum; j++) {
                        for (auto x : clusters_[j]) {
                            merged.push_back(std::move(x));
                        }
                    }

                    partial_sum = 0;

                    start_partial_sum = i + 1;
                    start_partial_index_ = start_partial_sum;
                    merged_clusters_.push_back(std::move(merged));
                }

            }

            if (start_partial_sum <= stop_partial_sum && any_partial && clusters_.size() > stop_partial_sum) {
                for (int k = start_partial_sum; k <= stop_partial_sum; k++) {
                    for (auto xx : clusters_[k]) {
                        if (big_clusters_.size() > 0) {
                            //big_clusters_[0]->push_back(std::move(xx));
                            for (auto p : xx->get_records()) {
                                big_clusters_[0]->push_back(p);

                            }
                            start_partial_index_ = k + 1;
                        } else if (merged_clusters_.size() > 0) {
                            merged_clusters_[0].push_back(std::move(xx));
                            start_partial_index_ = k + 1;
                        } else if (clusters_.size() > stop_partial_sum) {
                            clusters_[stop_partial_sum + 1].push_back(std::move(xx));
                            start_partial_index_ = k + 1;
                        } else {
                            // not even enough records to form 1 cluster.
                            // so there will be one partial cluster
                            partial_cluster_ = true;
                            start_partial_index_ = start_partial_sum;
                            stop_partial_index_ = stop_partial_sum;
                            break;
                        }
                    }
                }
            }
        }

        int32_t min_members_;
        bool partial_cluster_{false};
        int32_t start_partial_index_{0};
        int32_t stop_partial_index_{0};

        std::priority_queue<family *, std::vector<family *>, family_size_comparator> priority_queue_family_;
        std::vector<family *> big_clusters_;
        std::vector<std::vector<family *>> clusters_;
        std::vector<std::vector<family *>> merged_clusters_;
    };

    // chunky mmap reader
    // maps a chunk of mmap file reads and sends the records to handler class.

    template<typename HANDLER>
    class chunky_mmapreader {

    public:
        chunky_mmapreader(HANDLER &handler) : handler_{handler} {}

        void read_mmapfile(const std::string &filename) {

            struct stat stat_buf;
            long pagesz = sysconf(_SC_PAGESIZE);
            int fd = open(filename.c_str(), O_RDONLY);

            if (fd == -1) {
                throw std::runtime_error("cant mmap file");
            }

            off_t line_start = 0;
            char *file_chunk = nullptr;
            char *input_line = nullptr;
            off_t cur_off = 0;
            off_t map_offset = 0;
            size_t map_size = 1 * 1024 * 1024 + pagesz;
            if (fstat(fd, &stat_buf) == -1) {
                throw std::runtime_error("cant fstat file");
            }

            size_t length = stat_buf.st_size;

            if (map_offset + map_size > stat_buf.st_size) {
                map_size = stat_buf.st_size - map_offset;
            }

            file_chunk = static_cast<char *>(mmap(NULL, map_size, PROT_READ, MAP_PRIVATE, fd, map_offset));
            input_line = &file_chunk[cur_off];

            while (cur_off < stat_buf.st_size) {
                if (!(cur_off - map_offset < map_size)) {
                    munmap(file_chunk, map_size);
                    map_offset = (line_start / pagesz) * pagesz;
                    if (map_offset + map_size > stat_buf.st_size) {
                        map_size = stat_buf.st_size - map_offset;
                    }
                    file_chunk = static_cast<char *>(mmap(NULL, map_size, PROT_READ, MAP_PRIVATE, fd, map_offset));
                    input_line = &file_chunk[line_start - map_offset];
                }

                if (file_chunk[cur_off - map_offset] == '\n') {

                    std::string line(input_line, cur_off - line_start);
                    //std::cout << line << std::endl;
                    handler_.on_record(line);
                    line_start = cur_off + 1;
                    input_line = &file_chunk[line_start - map_offset];

                }
                cur_off++;
            }
        }

    protected:
        HANDLER &handler_;

    };
}

int main(int argc, char *argv[]) {
    using namespace agpc;

    if (argc != 3) {
        throw std::runtime_error("usage : ./FileReduce <absolute_path_input_file>  min_records");
    }
    family_manager fm;
    chunky_mmapreader<family_manager> mr(fm);
    std::string inputfilename = argv[1];
    std::string min_records_str = argv[2];
    int min_records = std::stoi(min_records_str);

    mr.read_mmapfile(inputfilename.c_str());

    clusterizer cl(min_records);
    cl.make_clusters(fm.get_groups());
    cl.write_files();
    return 0;
}
