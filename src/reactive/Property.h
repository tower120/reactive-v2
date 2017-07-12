#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <mutex>
#include <shared_mutex>

#include "ObserverBase.h"

#include "threading/SpinLock.h"
#include "threading/RWSpinLock.h"
#include "threading/dummy_mutex.h"

namespace reactive{
    template<class T>
    class Property {
    public:
        struct Data{
            std::vector<std::shared_ptr<ObserverBase>> observers;
            T value;

            //using Lock = threading::SpinLock<threading::SpinLockMode::Nonstop>;
            using Lock = threading::RWSpinLockWriterBiased<threading::SpinLockMode::Adaptive>;
            //using Lock = threading::dummy_mutex;
            //using Lock = std::shared_mutex;
            mutable Lock lock;

            void die(){
                // schedule death
                for(auto& observer : observers){
                    observer->property_died(this);
                    observer.reset();
                }
                observers.clear();
                observers.shrink_to_fit();
            }

            void set(const T& new_value){

                {
                    lock.lock();
                        value = new_value;
                    lock.unlock_and_lock_shared();
                        pulse();
                    lock.unlock_shared();
                }

            }

            void pulse(){
                // just for loop observers->run()
                // with self deletion

                int count = observers.size();
                int i = 0;
                while(i < count){
                    auto& observer = observers[i];
                    const bool do_unsubscribe_self = observer->run(/* observer */);

                    if (!do_unsubscribe_self){
                        i++;
                    } else {
                        // unordered delete O(1)
                        std::iter_swap(observers.begin()+i, observers.end() - 1);
                        observers.pop_back();

                        count--;
                    }
                }
            }

            void subscribe(const std::shared_ptr<ObserverBase>& observer) {
                observers.emplace_back(observer);
            }
            void unsubscribe(ObserverBase* observer) {
                do_unsubscribe(observer);
            }
            void do_unsubscribe(ObserverBase* observer){
                auto it = std::find_if(observers.begin(), observers.end(), [&](const auto& element) {
                    return (element.get() == observer);
                });

                // unordered delete O(1)
                if (it != observers.end()) {
                    std::iter_swap(it, observers.end() - 1);
                    observers.pop_back();
                }
            }

            const auto& get() const{
                std::shared_lock<Lock> l(lock);
                return value;
            }
        };
        std::shared_ptr<Data> data;
    public:
        // for observer
        const std::shared_ptr<Data>& getData() const {
            return data;
        }

        Property()
            :data(std::make_shared<Data>())
        {}

        void set(const T& new_value){
            data->set(new_value);
        }

        ~Property() {
            if (data) data->die();
        };
    };
}