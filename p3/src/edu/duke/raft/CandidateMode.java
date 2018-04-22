package edu.duke.raft;

public class CandidateMode extends RaftMode {
  public void go () {
    synchronized (mLock) {

      // self.term++; // Increment term to indicate start of new election
      // electionCountTimer.start();
      // Declare election
      // electionStart(); // This will invoke the requestVoteRPC of all other servers


      int term = 0;      
      System.out.println ("S" + 
			  mID + 
			  "." + 
			  term + 
			  ": switched to candidate mode.");
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


    }
  }
}
