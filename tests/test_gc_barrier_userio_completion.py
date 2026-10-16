import re
import subprocess
import sys
import tempfile
from pathlib import Path

repo = Path(sys.argv[1]).resolve()
source_file = 'src/ssd/Address_Mapping_Unit_Page_Level.cpp'
base_commit = '51f0f2d3fed92d88ef4a0fa61a38024b07bf9d16'
original = subprocess.check_output(['git', '-C', str(repo), 'show', f'{base_commit}:{source_file}'], text=True)
patched = (repo / source_file).read_text()

def extract(source):
    marker = 'inline void Address_Mapping_Unit_Page_Level::Remove_barrier_for_accessing_lpa('
    start = source.find(marker)
    if start < 0:
        raise RuntimeError('GC barrier function not found')
    begin = source.find('{', start)
    nesting = 0
    for pos in range(begin, len(source)):
        if source[pos] == '{':
            nesting += 1
        elif source[pos] == '}':
            nesting -= 1
            if nesting == 0:
                return source[start:pos + 1]
    raise RuntimeError('Unbalanced function braces')

cpp = r'''
#include <cstdint>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>
using stream_id_type = unsigned;
using LPA_type = uint64_t;
enum class Transaction_Source_Type { USERIO, MAPPING };
struct NVM_Transaction_Flash {
    Transaction_Source_Type Source = Transaction_Source_Type::USERIO;
    static int destroyed;
    ~NVM_Transaction_Flash() { ++destroyed; }
};
int NVM_Transaction_Flash::destroyed = 0;
struct Cache {
    int completed = 0;
    void Handle_transaction_serviced_from_barrier(NVM_Transaction_Flash* tr) {
        if (tr->Source == Transaction_Source_Type::USERIO) ++completed;
    }
};
struct FTL { Cache* Data_cache_manager; };
struct Domain {
    std::set<LPA_type> Locked_LPAs;
    std::multimap<LPA_type, NVM_Transaction_Flash*> Read_transactions_behind_LPA_barrier;
    std::multimap<LPA_type, NVM_Transaction_Flash*> Write_transactions_behind_LPA_barrier;
};
struct Address_Mapping_Unit_Page_Level {
    std::vector<Domain*> domains;
    FTL* ftl;
    int mapping_calls = 0;
    void handle_transaction_serviced_signal_from_PHY(NVM_Transaction_Flash* tr) {
        ++mapping_calls;
        if (tr->Source != Transaction_Source_Type::MAPPING) return;
    }
    void Remove_barrier_for_accessing_lpa(stream_id_type, LPA_type);
};
#define PRINT_ERROR(s) throw std::runtime_error(s);
__FUNCTION__
int main() {
    Domain d;
    Cache c;
    FTL f{&c};
    Address_Mapping_Unit_Page_Level unit;
    unit.ftl = &f;
    unit.domains.push_back(&d);
    d.Locked_LPAs.insert(42);
    d.Read_transactions_behind_LPA_barrier.emplace(42, new NVM_Transaction_Flash);
    d.Write_transactions_behind_LPA_barrier.emplace(42, new NVM_Transaction_Flash);
    unit.Remove_barrier_for_accessing_lpa(0, 42);
    std::cout << "cache_completed=" << c.completed
              << " mapping_calls=" << unit.mapping_calls
              << " destroyed=" << NVM_Transaction_Flash::destroyed << '\n';
    return c.completed == 2 && NVM_Transaction_Flash::destroyed == 2
           && d.Read_transactions_behind_LPA_barrier.empty()
           && d.Write_transactions_behind_LPA_barrier.empty() && d.Locked_LPAs.empty() ? 0 : 2;
}
'''
with tempfile.TemporaryDirectory() as tmp:
    results = {}
    for name, source in [('original', original), ('patched', patched)]:
        path = Path(tmp) / f'{name}.cpp'
        binary = Path(tmp) / name
        path.write_text(cpp.replace('__FUNCTION__', extract(source)))
        build = subprocess.run(['g++', '-std=c++11', '-O1', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all', str(path), '-o', str(binary)], capture_output=True, text=True)
        if build.returncode:
            raise RuntimeError(f'{name} compile failed:\n{build.stderr}')
        run = subprocess.run([str(binary)], capture_output=True, text=True)
        results[name] = run.returncode
        print(f'{name}: exit={run.returncode} {run.stdout.strip()}')
        if run.stderr:
            print(run.stderr, file=sys.stderr)
    if results != {'original': 2, 'patched': 0}:
        raise SystemExit(f'Unexpected results: {results}')
    print('TARGETED_DISPATCH_REGRESSION_PASS')
