package edu.duke.raft;

import java.util.Timer;
import java.util.concurrent.ThreadLocalRandom;

public class CandidateMode extends RaftMode {
	
	Timer electionCountTimer;
	int electionTimeout;
	final int electionTimerID = 2;
	
  public void go () {
    synchronized (mLock) {
    	log("switched to candidate mode.");
    	int term = mConfig.getCurrentTerm() + 1; // Increment term to indicate start of new election
    	mConfig.setCurrentTerm(term, mID);
    	
        if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
          electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
        }

        electionCountTimer = scheduleTimer(electionTimeout, electionTimerID);
        
        RaftResponses.setTerm(term);
        RaftResponses.clearVotes(term);
        for (int i = 0; i < mConfig.getNumServers(); i++) {
        	if (i != mID) {
        		this.remoteRequestVote(i, mConfig.getCurrentTerm(), mID, mLog.getLastIndex(), mLog.getLastTerm());
        	}
        }
        
        // if absolute majority of votes, transition to leader.
        //If AppendEntries RPC received from new leader: convert to
       // follower
        // If election timeout elapses: start new election
        
        
      // electionCountTimer.start();
      // Declare election
      // electionStart(); // This will invoke the requestVoteRPC of all other servers

    }
  }

  // @param candidate’s term
  // @param candidate requesting vote
  // @param index of candidate’s last log entry
  // @param term of candidate’s last log entry
  // @return 0, if server votes for candidate; otherwise, server's
  // current term 
  public int requestVote (int candidateTerm,
			  int candidateID,
			  int lastLogIndex,
			  int lastLogTerm) {
    synchronized (mLock) {

      // If in CandidateMode, already voted for self so return term
      // likely a bit more sophisiticated than this. 
      // return self.term

      int term = mConfig.getCurrentTerm ();
      int result = term;
      return result;
    }
  }
  

  // @param leader’s term
  // @param current leader
  // @param index of log entry before entries to append
  // @param term of log entry before entries to append
  // @param entries to append (in order of 0 to append.length-1)
  // @param index of highest committed entry
  // @return 0, if server appended entries; otherwise, server's
  // current term
  public int appendEntries (int leaderTerm,
			    int leaderID,
			    int prevLogIndex,
			    int prevLogTerm,
			    Entry[] entries,
			    int leaderCommit) {
    synchronized (mLock) {

      // Two cases: 1. Stale leader, 2. Stale candidate


      int term = mConfig.getCurrentTerm ();
      int result = term;
      return result;
    }
  }

  // @param id of the timer that timed out
  public void handleTimeout (int timerID) {
    synchronized (mLock) {
      // Either: 1. Won election (become leader), 2. Lost election (become follower)
      // 3. Still waiting on vote (reset timeout)
      // if(won)
      //    RaftModeImpl.switchMode((LeaderMode) self);
      // elif(lost)
      //  RaftModeImpl.switchMode((FollowerMode) self);
      //else
      //    electionCountTimer.reset();
    	if (timerID == electionTimerID) {
        	double[] votes = new double[mConfig.getNumServers()]; // why double? 
        	for (double v : votes) {
        		v = 0;
        	}
        	for (int i : RaftResponses.getVotes(mConfig.getCurrentTerm())) {
        		votes[i]++;
        	}
        	boolean lost = false;
        	for (int id = 0; id < votes.length; id++) {
        		if (id != mID && votes[id] > ((double) mConfig.getNumServers())/2) {
        			lost = true;
        		}
        	}
        		
        	if (votes[mID] > ((double) mConfig.getNumServers())/2) {
        		RaftServerImpl.setMode(new LeaderMode());
        	} else if (lost) {
        		RaftServerImpl.setMode(new FollowerMode());
        	} else {
        		electionCountTimer.cancel();
        		int electionTimeout; 
                if ((electionTimeout = mConfig.getTimeoutOverride()) == -1) {
                  electionTimeout = ThreadLocalRandom.current().nextInt(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
                }

                electionCountTimer = scheduleTimer(electionTimeout, electionTimerID);
        	}
    	}
    }
  }
  
public void log(String message) {
		int currentTerm = mConfig.getCurrentTerm();
		System.out.println("S" + mID + "." + currentTerm + ": " + message);
}
}
