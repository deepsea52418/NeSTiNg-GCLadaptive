void VlanEtherTrafGenGCL::receivePacket(Packet *msg)
    {
        // 这个我不知道有没有用，先禁用掉
        // EV_INFO << "Received packet `" << msg->getName() \
        // <<  "' length= " << msg->getByteLength() << "B\n" \
        // << "src: " << msg->getTag<MacAddressInd>()->getSrcAddress() \
        // << " " << "dest: " << msg->getTag<MacAddressInd>()->getDestAddress() << " " << "pcp_value: " \
        // << msg->getTag<EnhancedVlanInd>()->getPcp() << " " ;
        
        auto data = msg->peekData(); // get all data from the packet
        auto regions = data->getAllTags<CreationTimeTag>(); // get all tag regions

        // SimTime delay;
        simtime_t delay;
        for (auto& region : regions) { // for each region do
            auto creationTime = region.getTag()->getCreationTime(); // 获取数据包创建时间
            delay = simTime() - creationTime; // 计算延迟
            // EV_INFO << "e2e_delay" << delay; //暂时先禁用所有omnet中的日志信息
        }

        // 写入result_file
        this->result_file << "{ \"time\": "<<simTime() << ", \"src\": \"" << msg->getTag<MacAddressInd>()->getSrcAddress() << "\""\
            << ", \"dest\": \"" << msg->getTag<MacAddressInd>()->getDestAddress() << "\"" \
            << ", \"pcp\": " << msg->getTag<EnhancedVlanInd>()->getPcp() \
            << ", \"e2edelay\": " << delay << " } "   << endl  ;
                
        // asw 周期内累加制  begin 
        static Schedule<GateBitvector>* currentSchedule = new Schedule<GateBitvector>();
        // asw 周期内累加制  end

        static bool first = true;
        if (first)
        {
            // 获取当前的GCL
            Schedule<GateBitvector>* currentSchedule_1 = gateController->getCurrentSchedule();
            
            // asw 周期内累加制  begin 
            currentSchedule->setBaseTime(currentSchedule_1->getBaseTime());
            currentSchedule->setCycleTime(currentSchedule_1->getCycleTime());
            currentSchedule->addControlListEntry(currentSchedule_1->getTimeInterval(0), currentSchedule_1->getScheduledObject(0));
            currentSchedule->addControlListEntry(currentSchedule_1->getTimeInterval(1), currentSchedule_1->getScheduledObject(1));
            // asw 周期内累加制  end

            first = false;
        }

        
        // AWS 调节算法
        // 如果收到的报文PCP=7并且延迟大于75us
        if ( msg->getTag<EnhancedVlanInd>()->getPcp() == 7 && delay > SimTime(75, SIMTIME_US)) 
        {
            // Schedule<GateBitvector>* currentSchedule = gateController->getCurrentSchedule();
            Schedule<GateBitvector> nextSchedule;
            // GateBitvector bitvector = currentSchedule->getScheduledObject(0);
            simtime_t time_interval = currentSchedule->getTimeInterval(0);
            simtime_t schedule_cycle = currentSchedule->getCycleTime();
            
            // 两端预留10%, 100, 步进周期5%, 50us 

            // simtime_t是omnet中表示模拟时间的数据类型
            // trunc()函数将截取浮点数的整数部分
            // SIMTIME_US在omnet中表示微秒
            // 代码使用了time_interval.trunc(SIMTIME_US)函数将time_interval这个时间间隔截断为微秒级别的整数部分。
            // 然后，代码加上了10微秒的偏移量，得到一个稍微大一点的时间间隔。接下来，代码计算了schedule_cycle * 0.9这个值，并将其与刚刚计算出来的时间间隔进行比较。如果
            // 时间间隔比schedule_cycle * 0.9要大，那么代码就选择schedule_cycle * 0.9作为目标时间间隔。否则，代码就选择
            // 刚刚计算出来的时间间隔作为目标时间间隔。最终，代码得到了一个目标时间间隔的值，保存在target_time_interval变量中。
            // asw 周期内累加制  begin 
            simtime_t target_time_interval = ((time_interval.trunc(SIMTIME_US) + SimTime(10, SIMTIME_US)) > (schedule_cycle * 0.9)) ? (schedule_cycle * 0.9) :  (time_interval.trunc(SIMTIME_US) + SimTime(10, SIMTIME_US));
            // asw 周期内累加制  end

            this->result_file << "{ \"time\": "<< simTime() << ", \"src\": \"" << msg->getTag<MacAddressInd>()->getSrcAddress() << "\""\
            << ", \"dest\": \"" << msg->getTag<MacAddressInd>()->getDestAddress() << "\"" \
            << ", \"pcp\": " << msg->getTag<EnhancedVlanInd>()->getPcp() \
            << ", \"pcp=7-interval\": " << target_time_interval << ", \"pcp<7-interval\": " << SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US) << " } "<< endl;
           
            nextSchedule.addControlListEntry(target_time_interval.trunc(SIMTIME_US), currentSchedule->getScheduledObject(0));
            nextSchedule.addControlListEntry(SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US), currentSchedule->getScheduledObject(1));
            gateController->setNextSchedule(nextSchedule);

            // asw 周期内累加制  begin 
            currentSchedule->setTimeInterval(0, target_time_interval.trunc(SIMTIME_US));
            currentSchedule->setTimeInterval(1, SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US));
            // asw 周期内累加制  end 
        }
        
        // aws原论文将阈值设置为最差可接受时延的50%
        if ( msg->getTag<EnhancedVlanInd>()->getPcp() == 7 && delay < SimTime(75, SIMTIME_US))
        {
            // 修改调度时隙分配, 将流量分为两个, 优先级为7和优先级为其他
            // Schedule<GateBitvector>* currentSchedule = gateController->getCurrentSchedule();
            Schedule<GateBitvector> nextSchedule;
            // GateBitvector bitvector = currentSchedule->getScheduledObject(0);
            simtime_t time_interval = currentSchedule->getTimeInterval(0);
            simtime_t schedule_cycle = currentSchedule->getCycleTime();

            // asw 周期内累加制  begin 
            simtime_t target_time_interval = ((time_interval.trunc(SIMTIME_US) - SimTime(10, SIMTIME_US)) < (schedule_cycle * 0.1)) ? (schedule_cycle * 0.1) :  (time_interval.trunc(SIMTIME_US) - SimTime(10, SIMTIME_US));
            // asw 周期内累加制  end



            // 保证不小于总周期的10%, 步进10us, 预留周期10%, 也就是10us
            // simtime_t target_time_interval =  ( time_interval.trunc(SIMTIME_US) - SimTime(10, SIMTIME_US) ) < SimTime(40, SIMTIME_US) ? SimTime(40, SIMTIME_US) :  (time_interval.trunc(SIMTIME_US) - SimTime(10, SIMTIME_US));

            this->result_file << "{ \"time\": "<<simTime() << ", \"src\": \"" << msg->getTag<MacAddressInd>()->getSrcAddress() << "\""\
            << ", \"dest\": \"" << msg->getTag<MacAddressInd>()->getDestAddress() << "\"" \
            << ", \"pcp\": " << msg->getTag<EnhancedVlanInd>()->getPcp() \
            << ", \"pcp=7-interval\": " << target_time_interval << ", \"pcp<7-interval\": " << SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US) << " } "<< endl;

            nextSchedule.addControlListEntry(target_time_interval.trunc(SIMTIME_US), currentSchedule->getScheduledObject(0));
            nextSchedule.addControlListEntry(SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US), currentSchedule->getScheduledObject(1));
            gateController->setNextSchedule(nextSchedule);

            // asw 周期内累加制  begin 
            currentSchedule->setTimeInterval(0, target_time_interval.trunc(SIMTIME_US));
            currentSchedule->setTimeInterval(1, SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US));
            // asw 周期内累加制  end
        }
        
        
        /*
        // 反馈调节算法
        // 判断pcp=7的流量延时情况, 延迟 > 120us了(80%), 决定是否调整窗口, st流时延上限设为150us
        if ( msg->getTag<EnhancedVlanInd>()->getPcp() == 7 && delay > SimTime(110, SIMTIME_US)) 
        {
            // 修改调度时隙分配, 将流量分为两, 优先级为7和优先级为其他
            Schedule<GateBitvector>* currentSchedule = gateController->getCurrentSchedule();
            Schedule<GateBitvector> nextSchedule;
            GateBitvector bitvector = currentSchedule->getScheduledObject(0);
            simtime_t time_interval = currentSchedule->getTimeInterval(0);
            simtime_t schedule_cycle = currentSchedule->getCycleTime();
            double scale = delay.dbl() / SimTime(0.00005).dbl();

            // 保证不大于调度调节周期
            simtime_t target_time_interval =  ((scale * time_interval) > schedule_cycle * 0.95) ? (schedule_cycle * 0.95) :  (scale * time_interval);

            this->result_file << "{ \"time\": "<< simTime() << ", \"src\": \"" << msg->getTag<MacAddressInd>()->getSrcAddress() << "\""\
            << ", \"dest\": \"" << msg->getTag<MacAddressInd>()->getDestAddress() << "\"" \
            << ", \"pcp\": " << msg->getTag<EnhancedVlanInd>()->getPcp() \
            << ", \"pcp=7-interval\": " << target_time_interval << ", \"pcp<7-interval\": " << SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US) << " } "<< endl;

            // this->result_file << endl << "cycle " <<  schedule_cycle   << "   current time_interval: " << time_interval << "   ---------------0 internal " << target_time_interval << "=====1 internal   " << (SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US)) << endl;
            
            nextSchedule.addControlListEntry(target_time_interval.trunc(SIMTIME_US), currentSchedule->getScheduledObject(0));
            nextSchedule.addControlListEntry(SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US), currentSchedule->getScheduledObject(1));
            gateController->setNextSchedule(nextSchedule);
        }
        
        // 判断pcp=7的流量延时情况, 延迟阈值设置与0.2 * 最大可接受时延, 调小st流量窗口
        if ( msg->getTag<EnhancedVlanInd>()->getPcp() == 7 && delay < SimTime(20, SIMTIME_US)) 
        {
            // 修改调度时隙分配, 将流量分为两个, 优先级为7和优先级为其他
            Schedule<GateBitvector>* currentSchedule = gateController->getCurrentSchedule();
            Schedule<GateBitvector> nextSchedule;
            GateBitvector bitvector = currentSchedule->getScheduledObject(0);
            simtime_t time_interval = currentSchedule->getTimeInterval(0);
            simtime_t schedule_cycle = currentSchedule->getCycleTime();
            // 如果时延太小, 就把目标时延设置到75us的程度
            double scale = delay.dbl() / SimTime(0.00006).dbl();

            // this->result_file << "time_interval: " << time_interval << endl;

            // 保证不小于初始时间窗口
            simtime_t target_time_interval =  ((scale * time_interval).trunc(SIMTIME_US) < SimTime(40, SIMTIME_US)) ? SimTime(40, SIMTIME_US) :  (scale * time_interval).trunc(SIMTIME_US);
            
            this->result_file << "{ \"time\": "<<simTime() << ", \"src\": \"" << msg->getTag<MacAddressInd>()->getSrcAddress() << "\""\
            << ", \"dest\": \"" << msg->getTag<MacAddressInd>()->getDestAddress() << "\"" \
            << ", \"pcp\": " << msg->getTag<EnhancedVlanInd>()->getPcp() \
            << ", \"pcp=7-interval\": " << target_time_interval << ", \"pcp<7-interval\": " << SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US) << " } "<< endl;

            // target_time_interval = target_time_interval < SimTime(15, SIMTIME_US) ? SimTime(15, SIMTIME_US)
            // this->result_file << endl << "cycle " <<  schedule_cycle   << "   current time_interval: " << time_interval << "   ---------------0 internal " << target_time_interval << "=====1 internal   " << (SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US)) << endl;

            nextSchedule.addControlListEntry(target_time_interval.trunc(SIMTIME_US), currentSchedule->getScheduledObject(0));
            nextSchedule.addControlListEntry(SimTime(100, SIMTIME_US) - target_time_interval.trunc(SIMTIME_US), currentSchedule->getScheduledObject(1));
            gateController->setNextSchedule(nextSchedule);
        }
        */
        // 重写父类EtherTrafGen.cc中的receivePacket方法
        EV_INFO << "Received packet `" << msg->getName() << "' length= " << msg->getByteLength() << "B\n";
        packetsReceived++;
        emit(packetReceivedSignal, msg);
        delete msg;
    }
